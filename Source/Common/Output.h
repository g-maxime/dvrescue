/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#pragma once
#include "ProcessFile.h"
#include <string>
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
// Sizes
//***************************************************************************

const int Sta_Bits = 4;
const int Dseq_Bits = 4;
const int Sta_Size = 1 << Sta_Bits;
const int Dseq_Size = 1 << Dseq_Bits;
const int DseqSta_Size = Dseq_Size * Sta_Size;

//***************************************************************************
// DvDif_Analysis_Frame helpers
//***************************************************************************

struct frame_arb
{
public:
    frame_arb(decltype(MediaInfo_Event_DvDif_Analysis_Frame_1::Arb) Value) : _Value(Value) {}
    inline int Value() { return _Value & 0xF; }
    inline bool HasValue() { return _Value & (1 << 4); }
    inline bool Repeat() { return _Value & (1 << 5); }
    inline bool NonConsecutive() { return _Value & (1 << 6); }

private:
    decltype(MediaInfo_Event_DvDif_Analysis_Frame_1::Arb) _Value;
};

//***************************************************************************
// Arrays
//***************************************************************************

const size_t chroma_subsampling_size = 3;
extern const char* const chroma_subsampling[chroma_subsampling_size];

//***************************************************************************
// Formating helpers
//***************************************************************************

string timecode_to_string(int Seconds, bool DropFrame, int Frames);
string seconds_to_timestamp(double Seconds_Float);
string date_to_string(int Years, int Months, int Days);
char uint4_to_hex4(int Value);

//***************************************************************************
// Status of a frame
//***************************************************************************

bool Frame_HasErrors(const MediaInfo_Event_DvDif_Analysis_Frame_1& Frame);
inline bool Frame_HasErrors(const MediaInfo_Event_DvDif_Analysis_Frame_1* Frame) { return Frame_HasErrors(*Frame); }

class computed_errors
{
public:
    computed_errors();
    bool Compute(const MediaInfo_Event_DvDif_Analysis_Frame_1& Frame); // All Dseq
    bool Compute(const MediaInfo_Event_DvDif_Analysis_Frame_1& Frame, int Dseq); // Per Dseq

    // Global Data
    size_t Video_Sta_TotalPerSta[Sta_Size];
    size_t Video_Sta_EvenTotalPerSta[Sta_Size];
    size_t Audio_Data_Total;
    size_t Audio_Data_EvenTotal;

    // Per Dseq
    struct dseq
    {
        size_t Video_Sta_TotalPerSta[Sta_Size];
        size_t Video_Sta_EvenTotalPerSta[Sta_Size];
        size_t Audio_Data_Total;
        size_t Audio_Data_EvenTotal;
    };
    dseq PerDseq;
};
