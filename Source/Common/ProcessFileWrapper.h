/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#pragma once

#include <cstddef> //for std::size_t, native size_t isn't avaiable in obj-c++ mode
#include <cstdint>
#include <string>
#include <vector>

#include "ThirdParty/TimeCode/TimeCode.h"

class file;
#if defined(ENABLE_DECKLINK) || defined(ENABLE_SIMULATOR)
class matroska_writer;
#endif

//***************************************************************************
// Enums
//***************************************************************************

enum playback_mode {
    Playback_Mode_NotPlaying = 0,
    Playback_Mode_Playing
};

//***************************************************************************
// Structures
//***************************************************************************

#if defined(ENABLE_DECKLINK) || defined(ENABLE_SIMULATOR)
struct decklink_frame
{
    uint32_t                    Width = {};
    uint32_t                    Height = {};
    uint8_t*                    Video_Buffer = {};
    size_t                      Video_Buffer_Size = {};
    uint8_t*                    Audio_Buffer = {};
    size_t                      Audio_Buffer_Size = {};
    TimeCode                    TC = {};
};

struct matroska_output
{
    matroska_writer* Writer = nullptr;
    std::ofstream* Output = nullptr;
};
#endif

//***************************************************************************
// Class FileWrapper
//***************************************************************************

class FileWrapper {
public:
    FileWrapper(file* File); // Constructor for DV/MediaInfo Interface
    #if defined(ENABLE_DECKLINK) || defined(ENABLE_SIMULATOR)
    FileWrapper(int Width, int Height, int Framerate_Num, int Framerate_Den, int SampleRate, int Channels, bool Has_Timecode = false); // Constructor for Decklink/Matroska Interface
    ~FileWrapper();
    #endif
    void Parse_Buffer(const uint8_t* Buffer, std::size_t Buffer_Size);

 private:
    file* File;
    #if defined(ENABLE_DECKLINK) || defined(ENABLE_SIMULATOR)
    bool IsMatroska = false;
    std::vector<matroska_output> Outputs;
    #endif
};

//***************************************************************************
// Class BaseWrapper
//***************************************************************************

class BaseWrapper {
public:
    // Constructor/Destructor
    virtual ~BaseWrapper() = 0;

    // Functions
    virtual std::string GetStatus() = 0;
    virtual float GetSpeed() = 0;
    virtual playback_mode GetMode() = 0;
    virtual void CreateCaptureSession(FileWrapper* Wrapper) = 0;
    virtual void StartCaptureSession() = 0;
    virtual void StopCaptureSession() = 0;
    virtual void SetPlaybackMode(playback_mode Mode, float Speed) = 0;
    virtual bool WaitForSessionEnd(uint64_t Timeout) = 0;
};
inline BaseWrapper::~BaseWrapper() {}

//TODO: split Controller/Capture logics
class ControllerBaseWrapper {
public:
    // Constructor/Destructor
    virtual ~ControllerBaseWrapper() = 0;

    // Functions
    virtual std::string GetStatus() = 0;
    virtual float GetSpeed() = 0;
    virtual playback_mode GetMode() = 0;
    virtual void SetPlaybackMode(playback_mode Mode, float Speed) = 0;
};
inline ControllerBaseWrapper::~ControllerBaseWrapper() {}

//***************************************************************************
// Utils
//***************************************************************************

//---------------------------------------------------------------------------
static std::string to_string(float x)
{
    long long p = 0;
    char* s = (char*)&p + sizeof(long long) - 2;
    int decimals;
    int units;
    static int Divisor = 10; // 1 decimal
    if (x < 0)
    {
        decimals = (int)(x * -Divisor + 0.5) % Divisor;
        units = (int)(-1 * x);
    }
    else {
        decimals = (int)(x * Divisor + 0.5) % Divisor;
        units = (int)x;
    }

    *--s = (decimals % 10) + '0'; // 1 decimal
    *--s = '.';

    do
    {
        *--s = (units % 10) + '0';
        units /= 10;
    } while (units > 0);

    if (x < 0)
        *--s = '-';
    return std::string(s);
}
