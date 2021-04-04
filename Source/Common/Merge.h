/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include "TimeCode.h"
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
//
//***************************************************************************

extern vector<string> Merge_InputFileNames;
extern string Merge_OutputFileName;
extern string MergeInfo_OutputFileName;
extern uint8_t MergeInfo_Format;
extern uint8_t Verbosity;
extern uint8_t UseAbst;
struct MediaInfo_Event_DvDif_Analysis_Frame_1;
struct MediaInfo_Event_Global_Demux_4;

//---------------------------------------------------------------------------
class dv_merge
{
public:
    void AddFrameAnalysis(size_t Merge_FilePos, const MediaInfo_Event_DvDif_Analysis_Frame_1* FrameData);
    void AddFrameData(size_t Merge_FilePos, const uint8_t* Buffer, size_t Buffer_Size);
    void Finish();

    TimeCode TC;
    bool SwitchToFile0 = false;
};
