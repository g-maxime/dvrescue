/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Common/Output.h"
//---------------------------------------------------------------------------

//***************************************************************************
// Arrays
//***************************************************************************

extern const char* const chroma_subsampling[chroma_subsampling_size] =
{
    "4:1:1",
    "4:2:0",
    "4:2:2",
};

//***************************************************************************
// Formating helpers
//***************************************************************************

//---------------------------------------------------------------------------
string timecode_to_string(int Seconds, bool DropFrame, int Frames)
{
    string Value("00:00:00:00");
    Value[0] += Seconds / 36000; Seconds %= 36000;
    Value[1] += Seconds / 3600; Seconds %= 3600;
    Value[3] += Seconds / 600; Seconds %= 600;
    Value[4] += Seconds / 60; Seconds %= 60;
    Value[6] += Seconds / 10; Seconds %= 10;
    Value[7] += Seconds;
    if (Frames < 100)
    {
        if (DropFrame)
            Value[8] = ';';
        Value[9] += Frames / 10;
        Value[10] += Frames % 10;
    }
    else
        Value.resize(8);

    return Value;
}

//---------------------------------------------------------------------------
string seconds_to_timestamp(double Seconds_Float)
{
    if (Seconds_Float >= 360000)
        return string(); // Not supported
    string Value("00:00:00.000");
    auto Seconds = int(Seconds_Float);
    Seconds_Float -= Seconds;
    Seconds_Float *= 1000;
    auto MilliSeconds = int(Seconds_Float);
    Seconds_Float -= MilliSeconds;
    if (Seconds_Float >= 0.5)
        MilliSeconds++;
    Value[0] += Seconds / 36000; Seconds %= 36000;
    Value[1] += Seconds / 3600; Seconds %= 3600;
    Value[3] += Seconds / 600; Seconds %= 600;
    Value[4] += Seconds / 60; Seconds %= 60;
    Value[6] += Seconds / 10; Seconds %= 10;
    Value[7] += Seconds;
    Value[9] += MilliSeconds / 100; MilliSeconds %= 100;
    Value[10] += MilliSeconds / 10; MilliSeconds %= 10;
    Value[11] += MilliSeconds;

    return Value;
}

//---------------------------------------------------------------------------
string date_to_string(int Years, int Months, int Days)
{
    string Value("2000-00-00");
    if (Years >= 70) // Arbitrary decided
    {
        Value[0] = '1';
        Value[1] = '9';
    }
    Value[2] += Years / 10;
    Value[3] += Years % 10;
    Value[5] += Months / 10;
    Value[6] += Months % 10;
    Value[8] += Days / 10;
    Value[9] += Days % 10;

    return Value;
}

//---------------------------------------------------------------------------
char uint4_to_hex4(int Value)
{
    if (((unsigned int)Value) >= 16)
        return 'X';
    if (Value >= 10)
        return 'A' - 10 + Value;
    return '0' + Value;
}

//***************************************************************************
// Status of a frame
//***************************************************************************

bool Frame_HasErrors(const MediaInfo_Event_DvDif_Analysis_Frame_1& Frame)
{
    return Frame.Video_STA_Errors || Frame.Audio_Data_Errors
        || ((Frame.TimeCode >> 30) & 0x1) || ((Frame.RecordedDateTime1 >> 30) & 0x1) || (Frame.Arb & (1 << 6)); // Non consecutive
}

computed_errors::computed_errors()
{
    memset(this, 0, sizeof(*this));
}

bool computed_errors::Compute(const MediaInfo_Event_DvDif_Analysis_Frame_1& Frame)
{
    bool HasErrors = false;
    for (int Dseq = 0; Dseq < Dseq_Size; Dseq++)
        HasErrors |= Compute(Frame, Dseq);
    return HasErrors;
}

bool computed_errors::Compute(const MediaInfo_Event_DvDif_Analysis_Frame_1& Frame, int Dseq)
{
    memset(&PerDseq, 0, sizeof(PerDseq));
    bool HasErrors = false;
    for (auto Sta = 0; Sta < Sta_Size; Sta++)
    {
        if (Frame.Video_STA_Errors)
        {
            auto DseqSta = (Dseq << Sta_Bits) | Sta;
            const auto n = Frame.Video_STA_Errors[DseqSta];
            if (n)
            {
                if (!HasErrors)
                    HasErrors = true;
                PerDseq.Video_Sta_TotalPerSta[Sta] += n;
                Video_Sta_TotalPerSta[Sta] += n;
                if (!(Dseq % 2))
                {
                    PerDseq.Video_Sta_EvenTotalPerSta[Sta] += n;
                    Video_Sta_EvenTotalPerSta[Sta] += n;
                }
            }
        }
    }
    if (Frame.Audio_Data_Errors)
    {
        const auto n = Frame.Audio_Data_Errors[Dseq];
        if (n)
        {
            if (!HasErrors)
                HasErrors = true;
            PerDseq.Audio_Data_Total += n;
            Audio_Data_Total += n;
            if (!(Dseq % 2))
            {
                PerDseq.Audio_Data_EvenTotal += n;
                Audio_Data_EvenTotal += n;
            }
        }
    }

    return HasErrors;
}