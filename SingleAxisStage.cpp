// Thorlabs Kinesis device adapter for Micro-Manager
// Author: Mark A. Tsuchida
//
// Copyright 2019-2020 The Board of Regents of the University of Wisconsin
// System
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from this
// software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#define NOMINMAX

#include "SingleAxisStage.h"

#include "Connections.h"
#include "DeviceEnumeration.h"
#include "Errors.h"

#include <algorithm>
#include <cmath>
#include <limits>


namespace {
    char const* const PROP_StageType = "StageType";
    char const* const PROPVAL_StageTypeLinear = "Linear";
    char const* const PROPVAL_StageTypeRotational = "Rotational";
    char const* const PROP_DeviceUnitsPerMillimeter = "DeviceUnitsPerMillimeter";
    char const* const PROP_DeviceUnitsPerRevolution = "DeviceUnitsPerRevolution";
}


namespace {
    // TODO C++17 will have std::clamp()
    template <typename T>
    inline T clamp(T const& value, T const& lower, T const& upper) {
        return std::max(lower, std::min(upper, value));
    }

    template <typename T>
    inline int clamp_int(T const& value) {
        T const lower{ std::numeric_limits<int>::min() };
        T const upper{ std::numeric_limits<int>::max() };
        T ret = clamp(value, lower, upper);
        return static_cast<int>(ret);
    }
}


SingleAxisStage::SingleAxisStage(std::string const& name,
    std::string const& serialNo, short channel,
    std::shared_ptr<KinesisDeviceConnection> connection) :
    serialNo_{ serialNo },
    channel_{ channel },
    givenName_{ name },
    homed_(false),
    initialized_(false),
    retainedConnection_{ connection }
{
    for (auto const& item : KinesisErrorCodes()) {
        SetErrorText(ERR_OFFSET + item.first, item.second.c_str());
    }

    // In general, the user must tell us whether the stage is linear or
    // rotational. (As far as I can tell, *_GetMotorTravelMode() doesn't work
    // as expected.)

    switch (TypeIDOfSerialNo(serialNo)) {
    case TypeIDCageRotator:
        isRotational_ = true;
        break;
    }
    CreateStringProperty(PROP_StageType,
        isRotational_ ? PROPVAL_StageTypeRotational : PROPVAL_StageTypeLinear,
        false, nullptr, true);
    AddAllowedValue(PROP_StageType, PROPVAL_StageTypeLinear);
    AddAllowedValue(PROP_StageType, PROPVAL_StageTypeRotational);

    // In general, the user must tell us how to convert from physical to device
    // units. (As far as I can tell, there is no API to query the actuator lead
    // screw pitch, or even the device units per motor revolution (device units
    // are not equal to "steps"). And *_GetRealValueFromDeviceUnit() and
    // *_GetDeviceUnitFromRealValue() seem to always return an error.)

    // The defaults below are set to small values to prevent accidents. Known
    // defaults (taken from the Kinesis app) are given for integrated devices.

    double defaultDeviceUnitsPerMm = 1000.0;
    switch (TypeIDOfSerialNo(serialNo)) {
    case TypeIDLabJack050: defaultDeviceUnitsPerMm = 1228800.0; break;
    case TypeIDLabJack490: defaultDeviceUnitsPerMm = 134737.0; break;
    case TypeIDLongTravelStage: defaultDeviceUnitsPerMm = 409600.0; break;
    case TypeIDVerticalStage: defaultDeviceUnitsPerMm = 25050.0; break;
    }
    CreateFloatProperty(PROP_DeviceUnitsPerMillimeter,
        defaultDeviceUnitsPerMm, false, nullptr, true);

    double defaultDeviceUnitsPerRevolution = 360.0;
    switch (TypeIDOfSerialNo(serialNo)) {
    case TypeIDCageRotator:
        defaultDeviceUnitsPerRevolution = 49152000.0;
        break;
    }
    CreateFloatProperty(PROP_DeviceUnitsPerRevolution,
        defaultDeviceUnitsPerRevolution, false, nullptr, true);
  
}


SingleAxisStage::~SingleAxisStage() = default;


int
SingleAxisStage::Initialize() {
    if (initialized_)
        return DEVICE_OK;

    auto motorDrive = Connect();
    if (!motorDrive) // Shouldn't happen
        return DEVICE_ERR;
    if (!motorDrive->GetConnection()->IsValid()) {
        return ERR_OFFSET + motorDrive->GetConnection()->ConnectionError();
    }
    motorDrive_ = std::move(motorDrive);

    short err;
    /*
    // For what it's worth (doesn't seem to change anything)
    err = motorDrive_->RequestSettings();
    if (err)
        return ERR_OFFSET + err;
    */
    char stageType[MM::MaxStrLength];
    GetProperty(PROP_StageType, stageType);
    isRotational_ = stageType != std::string{ PROPVAL_StageTypeLinear };

    if (isRotational_) {
        double deviceUnitsPerRevolution;
        GetProperty(PROP_DeviceUnitsPerRevolution, deviceUnitsPerRevolution);
        deviceUnitsPerUm_ = deviceUnitsPerRevolution / 360.0;
    }
    else {
        double deviceUnitsPerMm;
        GetProperty(PROP_DeviceUnitsPerMillimeter, deviceUnitsPerMm);
        deviceUnitsPerUm_ = deviceUnitsPerMm / 1000.0;
    }

    // Start polling, which will keep position and status bits up to date.
    // Ensure we are immediately up to date by first requesting position and
    // status bits.

    err = motorDrive_->RequestPosition();
    if (err)
        return ERR_OFFSET + err;

    err = motorDrive_->RequestStatusBits();
    if (err)
        return ERR_OFFSET + err;

    bool ok = motorDrive_->StartPolling(pollingIntervalMs_);
    if (!ok) {
        LogMessage(("Failed to start polling for serial no " + serialNo_).c_str());
    }

    CDeviceUtils::SleepMs(100); // Ensure the above requests finished (100 ms cycle)

    if (!motorDrive_->IsChannelEnabled()) {
        // A call to XXX_EnableChannel was added to Thorlabs example code at
        // some point, but only for some devices. If this causes errors, we may
        // need to branch depending on device type. At least some devices
        // always start up disabled, so enabling _is_ necessary for those.
        err = motorDrive_->SetChannelEnabled(true);
        if (err)
            return ERR_OFFSET + err;
        didEnable_ = true;
    }

    // It would be here to include the createProperty to expose stuff
    CPropertyAction* pAct = new CPropertyAction(this, &SingleAxisStage::OnPositionChange);
    int ret = CreateFloatProperty("Position Degrees", 0.0, false, pAct, false);
    if (ret != DEVICE_OK)
        return ret;

    initialized_ = true;

    return DEVICE_OK;
}


int
SingleAxisStage::Shutdown() {
    if (didEnable_)
        motorDrive_->SetChannelEnabled(false);

    if (motorDrive_)
        motorDrive_->StopPolling();

    motorDrive_.reset();

    return DEVICE_OK;
}


void
SingleAxisStage::GetName(char* name) const {
    // Name format is ModelNo_SerialNo or ModelNo_SerialNo-Channel

    // There are two situations in which GetName() is called before
    // Initialize(): (1) During hardware configuration after
    // DetectInstalledDevices() and (2) During normal config loading.
    //
    // In case (1) we don't know the model number yet, so we make a temporary
    // connection (which we can because the Hub has already initialized the
    // Kinesis API).
    //
    // In case (2), the Hub has not been initialized yet, so we echo back the
    // name used to create this device.

    std::string n;
    if (!givenName_.empty()) {
        n = givenName_;
    }
    else {
        auto tmpMotorDrive = Connect();
        n = MakeName(tmpMotorDrive.get());
    }

    CDeviceUtils::CopyLimitedString(name, n.c_str());
}


bool
SingleAxisStage::Busy() {
    // We are busy if the motor is moving, which we get from the status bits.
    // However, the status bits are only updated every polling interval, so
    // they do not immediately indicate movement after we kick off a move. So
    // we need to unconditionally report "busy" for one polling interval after
    // starting a movement.

    auto now = GetCurrentMMTime();
    auto msSinceMovementStart = (now - lastMovementStart_).getMsec();

    // Add a little extra to minimize the chance of a race due to jitter in the
    // polling.
    if (msSinceMovementStart <= pollingIntervalMs_ + 10.0)
        return true;

    DWORD status = motorDrive_->GetStatusBits();
    return status & (
        MotorDrive::StatusBitsMovingCW |
        MotorDrive::StatusBitsMovingCCW |
        MotorDrive::StatusBitsJoggingCW |
        MotorDrive::StatusBitsJoggingCCW |
        MotorDrive::StatusBitsHoming
    );
}


int
SingleAxisStage::GetPositionUm(double& pos) {
    long steps;
    int err = GetPositionSteps(steps);
    if (err != DEVICE_OK)
        return err;

    pos = steps / deviceUnitsPerUm_;

    return DEVICE_OK;
}


int
SingleAxisStage::SetPositionUm(double pos) {
    double dSteps = std::round(pos * deviceUnitsPerUm_);
    int steps = clamp_int(dSteps);
    return SetPositionSteps(steps);
}

int SingleAxisStage::OnPositionChange(MM::PropertyBase* pProp, MM::ActionType eAct)
{
    if (eAct == MM::BeforeGet)
    {
        printf("Getting position of Wheel device\n");
        double pos;
        GetPositionUm(pos);
        pProp->Set(pos);        
    }
    else if (eAct == MM::AfterSet)
    {
        double pos;
        pProp->Get(pos);
        printf("Moving to position %lf\n", pos);
        SetPositionUm(pos);
    }

    return DEVICE_OK;
}


int
SingleAxisStage::GetPositionSteps(long& steps) {
    // TODO Does it make sense to use encoder position for non-stepper?
    steps = motorDrive_->GetPositionCounter();
    return DEVICE_OK;
}


int
SingleAxisStage::SetPositionSteps(long steps) {
    int iSteps = sizeof(steps) > sizeof(iSteps) ?
        clamp_int(steps) : static_cast<int>(steps);

    short err = motorDrive_->MoveToPosition(iSteps);
    if (err)
        return ERR_OFFSET + err;

    lastMovementStart_ = GetCurrentMMTime();

    return DEVICE_OK;
}


int
SingleAxisStage::Home() {
    if (!motorDrive_->CanHome())
        return DEVICE_UNSUPPORTED_COMMAND;
    int ret = DEVICE_OK;

    if (homed_)
        return ret;

    short err = motorDrive_->Home();
    if (err)
        return ERR_OFFSET + err;
    else
        homed_ = true;

    lastMovementStart_ = GetCurrentMMTime();

    return DEVICE_OK;
}


std::unique_ptr<MotorDrive>
SingleAxisStage::Connect() const {
    auto connection = MakeConnection(serialNo_);
    if (!connection) // Shouldn't happen
        return {};

    return MakeKinesisMotorDrive(connection, channel_);
}


std::string
SingleAxisStage::MakeName(MotorDrive* motorDrive) const {
    std::string name;

    if (motorDrive && motorDrive->GetConnection()->IsValid()) {
        name += motorDrive->GetModelNo();
    }
    else {
        name += "Error";
        if (motorDrive) {
            name += std::to_string(motorDrive->GetConnection()->ConnectionError());
        }
    }

    name += '_';
    name += serialNo_;

    if (channel_ > 0) {
        name += '-';
        name += std::to_string(channel_);
    }

    return name;
}
