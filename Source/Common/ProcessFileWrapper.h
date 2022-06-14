/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#pragma once

#include <cstddef> //for std::size_t, native size_t isn't avaiable in obj-c++ mode
#include <string>

class file;

//***************************************************************************
// Enums
//***************************************************************************

enum playback_mode {
    Playback_Mode_NotPlaying = 0,
    Playback_Mode_Playing
};

//***************************************************************************
// Class FileWrapper
//***************************************************************************

class FileWrapper {
public:
    FileWrapper(file* File);
    void Parse_Buffer(const std::uint8_t* Buffer, std::size_t Buffer_Size);

 private:
    file* File;
};

//***************************************************************************
// Class BaseWrapper
//***************************************************************************

class BaseWrapper {
public:
    // Constructor/Destructor
    virtual ~BaseWrapper() = 0;

    // Functions
    virtual void CreateCaptureSession(FileWrapper* Wrapper) = 0;
    virtual void StartCaptureSession() = 0;
    virtual void StopCaptureSession() = 0;
    virtual void SetPlaybackMode(playback_mode Mode, float Speed) = 0;
    virtual void WaitForSessionEnd() = 0;
};
inline BaseWrapper::~BaseWrapper() {}
