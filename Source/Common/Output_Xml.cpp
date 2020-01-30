/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Common/Output.h"
#include "Common/Output_Xml.h"
#include "Common/ProcessFile.h"
#include "ZenLib/Ztring.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
static void Dseq_Begin(string& Text, size_t o, int Dseq)
{
    Text.append(o, '\t');
    Text += "<dseq n=\"";
    Text += to_string(Dseq);
    Text += "\">\n";
}

//---------------------------------------------------------------------------
static void Dseq_End(string& Text, size_t o)
{
    Text.append(o, '\t');
    Text += "</dseq>\n";
}

//---------------------------------------------------------------------------
static void Sta_Element(string& Text, size_t o, int Sta, size_t n, size_t n_even = size_t(-1))
{
    if (!n)
        return;

    Text.append(o, '\t');
    Text += "<sta t=\"";
    Text += to_string(Sta);
    Text += "\" n=\"";
    Text += to_string(n);
    Text += "\"";
    if (n_even != size_t(-1))
    {
        Text += " n_even=\"";
        Text += to_string(n_even);
        Text += '\"';
    }
    Text += "/>\n";
}

//---------------------------------------------------------------------------
static void Sta_Elements(string& Text, size_t o, const size_t* const Stas, const size_t* const Stas_even = nullptr)
{
    for (auto Sta = 0; Sta < Sta_Size; Sta++)
    {
        auto n = Stas[Sta];
        auto n_even = Stas_even == nullptr ? size_t(-1) : Stas_even[Sta];
        Sta_Element(Text, o, Sta, n, n_even);
    }
}

//---------------------------------------------------------------------------
static void Aud_Element(string& Text, size_t o, size_t n, size_t n_even = size_t(-1))
{
    if (!n)
        return;

    Text.append(o, '\t');
    Text += "<aud n=\"";
    Text += to_string(n);
    Text += "\"";
    if (n_even != size_t(-1))
    {
        Text += " n_even=\"";
        Text += to_string(n_even);
        Text += '\"';
    }
    Text += "/>\n";
}

//***************************************************************************
// Output
//***************************************************************************

//---------------------------------------------------------------------------
string Output_Xml(std::vector<file*>& PerFile)
{
    string Text;

    // XML header
    Text += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<dvrescue xmlns=\"https://mediaarea.net/dvrescue\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"https://mediaarea.net/dvrescue https://mediaarea.net/dvrescue/dvrescue.xsd\" version=\"1.0\">\n"
        "\t<creator>\n"
        "\t\t<program>dvrescue</program>\n"
        "\t\t<version>" Program_Version "</version>\n"
        "\t</creator>\n";

    for (const auto& File : PerFile)
    {
        if (File->PerFrame.empty() || File->PerChange.empty())
            continue; // Show the file only if there is some DV content

        // Media header
        Text += "\t<media ref=\"" + Ztring(File->MI.Get(Stream_General, 0, __T("CompleteName"))).To_UTF8() + "\">\n";

        // By Frame - For each line
        auto FrameNumber_Max = File->PerFrame.size() - 1;
        auto PerChange_Next = File->PerChange.begin();
        auto ShowFrames = true;
        for (const auto& Frame : File->PerFrame)
        {
            decltype(FrameNumber_Max) FrameNumber = &Frame - &*File->PerFrame.begin();
            auto ShowFrame = ShowFrames || FrameNumber == FrameNumber_Max || Frame_HasErrors(Frame);

            if (ShowFrames)
            {
                ShowFrames = false;

                if (FrameNumber)
                    Text += "\t\t</frames>\n";

                const auto Change = *PerChange_Next;
                PerChange_Next++;
                Text += "\t\t<frames";
                {
                    auto FrameCount = (PerChange_Next != File->PerChange.end() ? (*PerChange_Next)->FrameNumber : (FrameNumber_Max + 1)) - FrameNumber;
                    Text += " count=\"";
                    Text += to_string(FrameCount);
                    Text += '\"';
                }
                {
                    auto TimeStamp_Begin = FrameNumber / File->FrameRate;
                    Text += " pts=\"";
                    Text += seconds_to_timestamp(TimeStamp_Begin);
                    Text += '\"';
                }
                {
                    auto TimeStamp_End = (PerChange_Next != File->PerChange.end() ? (*PerChange_Next)->FrameNumber : (FrameNumber_Max + 1)) / File->FrameRate;
                    Text += " end_pts=\"";
                    Text += seconds_to_timestamp(TimeStamp_End);
                    Text += '\"';
                }
                if (Change->Width && Change->Height)
                {
                    Text += " size=\"";
                    Text += to_string(Change->Width);
                    Text += 'x';
                    Text += to_string(Change->Height);
                    Text += '\"';
                }
                if (Change->VideoRate_N)
                {
                    Text += " video_rate=\"";
                    Text += to_string(Change->VideoRate_N);
                    if (Change->VideoRate_D && Change->VideoRate_D != 1)
                    {
                        Text += '/';
                        Text += to_string(Change->VideoRate_D);
                    }
                    Text += '\"';
                }
                if (Change->VideoChromaSubsampling <= chroma_subsampling_size)
                {
                    Text += " chroma_subsampling=\"";
                    Text += chroma_subsampling[Change->VideoChromaSubsampling];
                    Text += '\"';
                }
                if (Change->VideoRatio_N)
                {
                    Text += " aspect_ratio=\"";
                    Text += to_string(Change->VideoRatio_N);
                    if (Change->VideoRatio_D && Change->VideoRatio_D != 1)
                    {
                        Text += '/';
                        Text += to_string(Change->VideoRatio_D);
                    }
                    Text += '\"';
                }
                if (Change->AudioRate_N)
                {
                    Text += " audio_rate=\"";
                    Text += to_string(Change->AudioRate_N);
                    if (Change->AudioRate_D && Change->AudioRate_D != 1)
                    {
                        Text += '/';
                        Text += to_string(Change->AudioRate_D);
                    }
                    Text += '\"';
                }
                if (Change->AudioChannels)
                {
                    Text += " channels=\"";
                    Text += to_string(Change->AudioChannels);
                    Text += '\"';
                }
                Text += ">\n";
            }

            if (PerChange_Next != File->PerChange.end() && (*PerChange_Next)->FrameNumber == FrameNumber + 1)
            {
                ShowFrame = true;
                ShowFrames = true; // For next frame
            }

            if (ShowFrame)
            {
                auto TimeStamp = FrameNumber / File->FrameRate;

                Text += "\t\t\t<frame";
                {
                    Text += " n=\"";
                    Text += to_string(FrameNumber);
                    Text += '\"';
                }
                {
                    Text += " pts=\"";
                    Text += seconds_to_timestamp(TimeStamp);
                    Text += '\"';
                }

                // TimeCode
                auto TimeCode_Seconds = (Frame->TimeCode >> 8) & 0x7FFFF; // Value
                if (TimeCode_Seconds != 0x7FFFF)
                {
                    auto TimeCode_DropFrame = Frame->TimeCode & 0x00000080 ? true : false;
                    auto TimeCode_Frames = Frame->TimeCode & 0x3F;
                    Text += " tc=\"" + timecode_to_string(TimeCode_Seconds, TimeCode_DropFrame, TimeCode_Frames) + "\"";
                }
                if ((Frame->TimeCode >> 31) & 0x1) // Repeat
                {
                    Text += " tc_r=\"1\"";
                }
                if ((Frame->TimeCode >> 30) & 0x1) // Non consecutive
                {
                    Text += " tc_nc=\"1\"";
                }

                // RecDate/RecTime
                string RecDateTime_String;
                auto RecDateTime_Years = (Frame->RecordedDateTime1 >> 17) & 0x7F;
                if (RecDateTime_Years != 0x7F)
                {
                    auto RecDateTime_Months = (Frame->RecordedDateTime2 >> 12) & 0x0F;
                    auto RecDateTime_Days = (Frame->RecordedDateTime2 >> 8) & 0x1F;
                    RecDateTime_String = date_to_string(RecDateTime_Years, RecDateTime_Months, RecDateTime_Days);
                }
                auto RecDateTime_Seconds = Frame->RecordedDateTime1 & 0x1FFFF;
                if (RecDateTime_Seconds != 0x1FFFF)
                {
                    if (!RecDateTime_String.empty())
                        RecDateTime_String += ' ';
                    auto RecDateTime_DropFrame = Frame->TimeCode & 0x00000080 ? true : false;
                    auto RecDateTime_Frames = Frame->RecordedDateTime2 & 0x7F;
                    RecDateTime_String += timecode_to_string(RecDateTime_Seconds, RecDateTime_DropFrame, RecDateTime_Frames);
                }
                if (!RecDateTime_String.empty())
                    Text += " rdt=\"" + RecDateTime_String + "\"";
                if ((Frame->RecordedDateTime1 >> 31) & 0x1) // Repeat
                {
                    Text += " rdt_r=\"1\"";
                }
                if ((Frame->RecordedDateTime1 >> 30) & 0x1) // Non consecutive
                {
                    Text += " rdt_nc=\"1\"";
                }
                if (Frame->RecordedDateTime1&(1 << 29)) // Start
                {
                    Text += " rec_start=\"1\"";
                }
                if (Frame->RecordedDateTime1&(1 << 28)) // End
                {
                    Text += " rec_end=\"1\"";
                }

                // Arb
                auto Arb = frame_arb(Frame->Arb);
                if (Arb.HasValue())
                {
                    Text += string(" arb=\"") + uint4_to_hex4(Arb.Value()) + "\"";
                }
                if (Arb.Repeat())
                {
                    Text += " arb_r=\"1\"";
                }
                if (Arb.NonConsecutive())
                {
                    Text += " arb_nc=\"1\"";
                }

                // Errors
                if (Frame->Video_STA_Errors || Frame->Audio_Data_Errors)
                {
                    Text += ">\n";

                    // Split
                    if (Frame->Video_STA_Errors_Count == DseqSta_Size || Frame->Audio_Data_Errors_Count == Dseq_Size)
                    {
                        // Compute
                        computed_errors ComputedErrors;

                        for (int Dseq = 0; Dseq < Dseq_Size; Dseq++)
                        {
                            if (ComputedErrors.Compute(*Frame, Dseq))
                            {
                                // Display
                                Dseq_Begin(Text, 4, Dseq);
                                Sta_Elements(Text, 5, ComputedErrors.PerDseq.Video_Sta_TotalPerSta);
                                Aud_Element(Text, 5, ComputedErrors.PerDseq.Audio_Data_Total);
                                Dseq_End(Text, 4);
                            }
                        }

                        // Display
                        Sta_Elements(Text, 4, ComputedErrors.Video_Sta_TotalPerSta, ComputedErrors.Video_Sta_EvenTotalPerSta);
                        Aud_Element(Text, 4, ComputedErrors.Audio_Data_Total, ComputedErrors.Audio_Data_EvenTotal);
                    }

                    Text += "\t\t\t</frame>\n";
                }
                else
                    Text += "/>\n";
            }
        }

        // Media footer
        Text += "\t\t</frames>\n";
        Text += "\t</media>\n";
    }

    // XML footer
    Text += "</dvrescue>\n";

    return Text;
}
