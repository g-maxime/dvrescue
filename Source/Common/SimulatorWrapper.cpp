/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE.txt file in the root of the source tree.
 */

#include "Common/SimulatorWrapper.h"
#include "ZenLib/File.h"
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;
using namespace ZenLib;

//---------------------------------------------------------------------------
struct ctl
{
    playback_mode               Mode = Playback_Mode_Playing;
    float                       Speed = 1.0;

    FileWrapper*                Wrapper = nullptr;
    bool                        IsCapturing = false;
    size_t                      F_Pos = 0;
    vector<File*>               F;
    mutex                       Mutex;
};

//---------------------------------------------------------------------------
SimulatorWrapper::SimulatorWrapper(const ZenLib::Ztring& FileName)
{
    auto P = new ctl;

    if (File::Exists(FileName))
        P->F.push_back(new File(FileName));
    for (size_t i = 0;; i++)
    {
        auto FileNameExt = FileName + __T('.') + Ztring::ToZtring(i);
        if (!File::Exists(FileNameExt))
            break;
        P->F.push_back(new File(FileNameExt));
    }

    Ctl = P;
}

//---------------------------------------------------------------------------
void SimulatorWrapper::CreateCaptureSession(FileWrapper* Wrapper)
{
    auto P = (ctl*)Ctl;

    P->Wrapper = Wrapper;
}

//---------------------------------------------------------------------------
void SimulatorWrapper::StartCaptureSession()
{
    auto P = (ctl*)Ctl;

    P->IsCapturing = true;
}

//---------------------------------------------------------------------------
void SimulatorWrapper::StopCaptureSession()
{
    auto P = (ctl*)Ctl;

    P->IsCapturing = false;
}

//---------------------------------------------------------------------------
void SimulatorWrapper::WaitForSessionEnd()
{
    auto P = (ctl*)Ctl;

    int8u* Buffer = new int8u[120000];
    for (;;)
    {
        if (!P->IsCapturing || P->Mode == Playback_Mode_NotPlaying)
            break;
        if (!P->Speed)
        {
            this_thread::yield();
            continue;
        }
        this_thread::sleep_for(std::chrono::microseconds((int)(1000000.0/(30000.0/1.001)/abs(P->Speed))));

        P->Mutex.lock();

        // Read next data
        if (P->Speed < 0)
        {
            if (P->F[P->F_Pos]->Position_Get() < 120000 * 2)
                P->F[P->F_Pos]->GoTo(0, File::FromBegin);
            else
                P->F[P->F_Pos]->GoTo(-120000 * 2, File::FromCurrent);
        }
        auto BytesRead = P->F[P->F_Pos]->Read(Buffer, 120000);

        P->Mutex.unlock();

        if (BytesRead != 120000)
            break;
        P->Wrapper->Parse_Buffer(Buffer, 120000);
    }

    delete[] Buffer;
}

//---------------------------------------------------------------------------
void SimulatorWrapper::SetPlaybackMode(playback_mode Mode, float Speed)
{
    auto P = (ctl*)Ctl;

    // No update if no mode/speed change
    if (P->Mode == Mode && P->Speed == Speed)
        return;

    P->Mutex.lock();

    // Update
    P->Mode = Mode;
    P->Speed = Speed;

    // Switch to next file
    auto SeekPos = P->F[P->F_Pos]->Position_Get();
    P->F_Pos++;
    if (P->F_Pos >= P->F.size())
        P->F_Pos = 0;
    P->F[P->F_Pos]->GoTo(SeekPos);

    P->Mutex.unlock();
}

//---------------------------------------------------------------------------
SimulatorWrapper::~SimulatorWrapper()
{
    delete Ctl;
}
