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

#include "BSC202_FW.h"

#include "DLLAccess.h"

#include "Thorlabs.MotionControl.Benchtop.StepperMotor.h"


static DLLAccess kinesisDll{ "Thorlabs.MotionControl.Benchtop.StepperMotor.dll" };


bool
BSC202_FWAccess::IsKinesisDriverAvailable() {
    return kinesisDll.IsValid();
}


short
BSC202_FWAccess::Kinesis_Open() {
    STATIC_DLL_FUNC(kinesisDll, SBC_Open, func);
    return func(CSerialNo());
}


short
BSC202_FWAccess::Kinesis_Close() {
    STATIC_DLL_FUNC(kinesisDll, SBC_Close, func);
    func(CSerialNo());
    return 0;
}


short
BSC202_FWAccess::Kinesis_GetNumChannels() {
    STATIC_DLL_FUNC(kinesisDll, SBC_GetNumChannels, func);
    return func(CSerialNo());
}


short
BSC202_FW::Kinesis_RequestSettings() {
    STATIC_DLL_FUNC(kinesisDll, SBC_RequestSettings, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_RequestStatusBits() {
    STATIC_DLL_FUNC(kinesisDll, SBC_RequestStatusBits, func);
    return func(CSerialNo(), Channel());
}


bool
BSC202_FW::Kinesis_StartPolling(int intervalMs) {
    STATIC_DLL_FUNC(kinesisDll, SBC_StartPolling, func);
    return func(CSerialNo(), Channel(), intervalMs);
}


void
BSC202_FW::Kinesis_StopPolling() {
    STATIC_DLL_FUNC(kinesisDll, SBC_StopPolling, func);
    func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_GetHardwareInfo(char* modelNo, DWORD sizeOfModelNo,
    WORD* type, WORD* numChannels, char* notes, DWORD sizeOfNotes,
    DWORD* firmwareVersion, WORD* hardwareVersion, WORD* modificationState) {

    STATIC_DLL_FUNC(kinesisDll, SBC_GetHardwareInfo, func);
    return func(CSerialNo(), Channel(), modelNo, sizeOfModelNo,
        type, numChannels, notes, sizeOfNotes, firmwareVersion,
        hardwareVersion, modificationState);
}


DWORD
BSC202_FW::Kinesis_GetStatusBits() {
    STATIC_DLL_FUNC(kinesisDll, SBC_GetStatusBits, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_EnableChannel() {
    STATIC_DLL_FUNC(kinesisDll, SBC_EnableChannel, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_DisableChannel() {
    STATIC_DLL_FUNC(kinesisDll, SBC_DisableChannel, func);
    return func(CSerialNo(), Channel());
}


int
BSC202_FW::Kinesis_GetMotorTravelMode() {
    STATIC_DLL_FUNC(kinesisDll, SBC_GetMotorTravelMode, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_SetMotorTravelMode(int mode) {
    STATIC_DLL_FUNC(kinesisDll, SBC_SetMotorTravelMode, func);
    return func(CSerialNo(), Channel(), static_cast<MOT_TravelModes>(mode));
}


short
BSC202_FW::Kinesis_ResetRotationModes() {
    STATIC_DLL_FUNC(kinesisDll, SBC_ResetRotationModes, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_SetRotationModes(int mode, int direction) {
    STATIC_DLL_FUNC(kinesisDll, SBC_SetRotationModes, func);
    return func(CSerialNo(), Channel(), static_cast<MOT_MovementModes>(mode),
        static_cast<MOT_MovementDirections>(direction));
}


short
BSC202_FW::Kinesis_RequestPosition() {
    STATIC_DLL_FUNC(kinesisDll, SBC_RequestPosition, func);
    return func(CSerialNo(), Channel());
}


int
BSC202_FW::Kinesis_GetPosition() {
    STATIC_DLL_FUNC(kinesisDll, SBC_GetPosition, func);
    return func(CSerialNo(), Channel());
}


long
BSC202_FW::Kinesis_GetPositionCounter() {
    STATIC_DLL_FUNC(kinesisDll, SBC_GetPositionCounter, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_MoveToPosition(int index) {
    STATIC_DLL_FUNC(kinesisDll, SBC_MoveToPosition, func);
    return func(CSerialNo(), Channel(), index);
}


bool
BSC202_FW::Kinesis_CanHome() {
    STATIC_DLL_FUNC(kinesisDll, SBC_CanHome, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_Home() {
    STATIC_DLL_FUNC(kinesisDll, SBC_Home, func);
    return func(CSerialNo(), Channel());
}


short
BSC202_FW::Kinesis_GetRealValueFromDeviceUnit(int deviceUnits,
    double* realValue, int unitType) {

    STATIC_DLL_FUNC(kinesisDll, SBC_GetRealValueFromDeviceUnit, func);
    return func(CSerialNo(), Channel(), deviceUnits, realValue, unitType);
}


short
BSC202_FW::Kinesis_GetDeviceUnitFromRealValue(double realValue,
    int* deviceUnits, int unitType) {

    STATIC_DLL_FUNC(kinesisDll, SBC_GetDeviceUnitFromRealValue, func);
    return func(CSerialNo(), Channel(), realValue, deviceUnits, unitType);
}
