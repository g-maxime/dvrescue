/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a Unlicense license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Common/Core.h"
#include "ZenLib/Ztring.h" // TODO: remove that
#include <cfenv>
using namespace ZenLib;
//---------------------------------------------------------------------------


//***************************************************************************
// Internal
//***************************************************************************

class file
{
public:
    MediaInfo MI;
    std::vector<MediaInfo_Event_DvDif_Analysis_Frame_0*> PerFrame;
    float FrameRate;

    file(const String& FileName);
    ~file();

    void AddFrame(const MediaInfo_Event_DvDif_Analysis_Frame_0* FrameData);
};

//***************************************************************************
// Callback
//***************************************************************************

void __stdcall Event_CallBackFunction(unsigned char* Data_Content, size_t Data_Size, void* UserHandler_Void)
{
    //Retrieving UserHandler
    file*                               UserHandler = (file*)UserHandler_Void;
    struct MediaInfo_Event_Generic*     Event_Generic = (struct MediaInfo_Event_Generic*) Data_Content;
    unsigned char                       ParserID;
    unsigned short                      EventID;
    unsigned char                       EventVersion;

    //Integrity test
    if (Data_Size < 4)
        return; //There is a problem

    //Retrieving EventID
    ParserID = (unsigned char)((Event_Generic->EventCode & 0xFF000000) >> 24);
    EventID = (unsigned short)((Event_Generic->EventCode & 0x00FFFF00) >> 8);
    EventVersion = (unsigned char)(Event_Generic->EventCode & 0x000000FF);
    switch (ParserID)
    {
    case MediaInfo_Parser_DvDif:
        switch (EventID)
        {
        case MediaInfo_Event_DvDif_Analysis_Frame: if (EventVersion == 0 && Data_Size == sizeof(struct MediaInfo_Event_DvDif_Analysis_Frame_0)) UserHandler->AddFrame((MediaInfo_Event_DvDif_Analysis_Frame_0*)Event_Generic); break;
        }
        break;
    }
}

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
file::file(const String& FileName)
{
    MI.Option(__T("File_Event_CallBackFunction"), __T("CallBack=memory://") + Ztring::ToZtring((size_t)&Event_CallBackFunction) + __T(";UserHandler=memory://") + Ztring::ToZtring((size_t)this));
    MI.Option(__T("File_DvDif_Analysis"), __T("1"));
    MI.Open(FileName);

    // Filing some info
    FrameRate = Ztring(MI.Get(Stream_Video, 0, __T("FrameRate_Original"))).To_float32();
    if (!FrameRate)
        FrameRate = Ztring(MI.Get(Stream_Video, 0, __T("FrameRate"))).To_float32();
    if (!FrameRate)
        FrameRate = float(30 / 1.001); // Default if no frame rate available
}

//---------------------------------------------------------------------------
file::~file()
{
    for (auto& Frame : PerFrame)
    {
        delete[] Frame->Errors;
        delete[] Frame->Video_STA_Errors;
    }
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
void file::AddFrame(const MediaInfo_Event_DvDif_Analysis_Frame_0* FrameData)
{
    MediaInfo_Event_DvDif_Analysis_Frame_0* ToPush = new MediaInfo_Event_DvDif_Analysis_Frame_0();
    std::memcpy(ToPush, FrameData, sizeof(MediaInfo_Event_DvDif_Analysis_Frame_0));
    if (FrameData->Errors)
    {
        size_t SizeToCopy = std::strlen(FrameData->Errors) + 1;
        ToPush->Errors = new char[SizeToCopy];
        std::memcpy(ToPush->Errors, FrameData->Errors, SizeToCopy);
    }
    if (FrameData->Video_STA_Errors)
    {
        size_t SizeToCopy = 256 * sizeof(size_t);
        ToPush->Video_STA_Errors = new size_t[SizeToCopy];
        std::memcpy(ToPush->Video_STA_Errors, FrameData->Video_STA_Errors, SizeToCopy);
    }
    PerFrame.push_back(ToPush);
}

//***************************************************************************
// Helpers
//***************************************************************************

//---------------------------------------------------------------------------
string TimeCode2String(int Seconds, bool DropFrame, int Frames)
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
string TimeStamp2String(float Seconds_Float)
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
string Date2String(int Years, int Months, int Days)
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

//***************************************************************************
// Constructor/Destructor
//***************************************************************************

//---------------------------------------------------------------------------
Core::Core()
{
    MediaInfo::Option_Static(__T("ParseSpeed"), __T("1.000"));
}

Core::~Core()
{
    PerFile_Clear();
}

//***************************************************************************
// Process
//***************************************************************************

//---------------------------------------------------------------------------
void Core::Process()
{
    PerFile_Clear();
    
    PerFile.reserve(Inputs.size());
    for (const auto& Input : Inputs)
    {
        PerFile.push_back(new file(Input));
    }
}

//---------------------------------------------------------------------------
float Core::State ()
{
    size_t Total = 0;
    for (const auto& File : PerFile)
    {
        Total += File->MI.State_Get();
    }
    return (((float)Total)/PerFile.size()/10000);
}

//***************************************************************************
// Output
//***************************************************************************

//---------------------------------------------------------------------------
string Core::OutputXml()
{
    string Text;

    // XML header
    Text += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<dvrescue xmlns=\"https://mediaarea.net/dvrescue\" version=\"1.0\">\n"
        "\t<creator>\n"
        "\t\t<program>dvrescue</program>\n"
        "\t\t<version>" Program_Version "</version>\n"
        "\t</creator>\n";

    for (const auto& File : PerFile)
    {
        if (File->PerFrame.empty())
            continue; // Show the file only if there is some DV content

        // Media header
        Text += "\t<media ref=\"" + Ztring(File->MI.Get(Stream_General, 0, __T("CompleteName"))).To_UTF8() + "\">\n";
        Text += "\t\t<frames>\n";

        // By Frame - For each line
        auto FrameNumber_Max = File->PerFrame.size() - 1;
        for (const auto& Frame : File->PerFrame)
        {
            decltype(FrameNumber_Max) FrameNumber = &Frame - &*File->PerFrame.begin();

            if (Frame->Errors || FrameNumber==0 || FrameNumber + 1 == FrameNumber_Max)
            {
                auto TimeStamp = FrameNumber / File->FrameRate;

                Text += "\t\t\t<frame"
                    " n=\"" + to_string(FrameNumber) + "\""
                    " pts=\"" + TimeStamp2String(TimeStamp) + "\"";

                // TimeCode
                auto TimeCode_Seconds = (Frame->TimeCode >> 8) & 0x7FFFF; // Value
                if (TimeCode_Seconds != 0x7FFFF)
                {
                    auto TimeCode_DropFrame = Frame->TimeCode & 0x00000080 ? true : false;
                    auto TimeCode_Frames = Frame->TimeCode & 0x3F;
                    Text += " timecode=\"" + TimeCode2String(TimeCode_Seconds, TimeCode_DropFrame, TimeCode_Frames) + "\"";
                }
                if ((Frame->TimeCode >> 31) & 0x1) // Repeat
                {
                    Text += " timecode_r=\"1\"";
                }
                if ((Frame->TimeCode >> 30) & 0x1) // Non consecutive
                {
                    Text += " timecode_nc=\"1\"";
                }

                // RecDate/RecTime
                string RecDateTime_String;
                auto RecDateTime_Years = (Frame->RecordedDateTime1 >> 17) & 0x7F;
                if (RecDateTime_Years != 0x7F)
                {
                    auto RecDateTime_Months = (Frame->RecordedDateTime2 >> 12) & 0x0F;
                    auto RecDateTime_Days = (Frame->RecordedDateTime2 >> 8) & 0x1F;
                    RecDateTime_String = Date2String(RecDateTime_Years, RecDateTime_Months, RecDateTime_Days);
                }
                auto RecDateTime_Seconds = Frame->RecordedDateTime1 & 0x1FFFF;
                if (RecDateTime_Seconds != 0x1FFFF)
                {
                    if (!RecDateTime_String.empty())
                        RecDateTime_String += ' ';
                    auto RecDateTime_DropFrame = Frame->TimeCode & 0x00000080 ? true : false;
                    auto RecDateTime_Frames = Frame->RecordedDateTime2 & 0x7F;
                    RecDateTime_String += TimeCode2String(RecDateTime_Seconds, RecDateTime_DropFrame, RecDateTime_Frames);
                }
                if (!RecDateTime_String.empty())
                    Text += " recdatetime=\"" + RecDateTime_String + "\"";
                if ((Frame->RecordedDateTime1 >> 31) & 0x1) // Repeat
                {
                    Text += " recdatetime_r=\"1\"";
                }
                if ((Frame->RecordedDateTime1 >> 30) & 0x1) // Non consecutive
                {
                    Text += " recdatetime_nc=\"1\"";
                }
                if (Frame->RecordedDateTime1&(1 << 29)) // Start
                {
                    Text += " recdatetime_start=\"1\"";
                }
                if (Frame->RecordedDateTime1&(1 << 28)) // End
                {
                    Text += " recdatetime_end=\"1\"";
                }

                // Arb
                auto Arb = Frame->Arb;
                if (Arb & (1 << 4)) // Value
                {
                    auto Arb_Value = Arb & 0xF;
                    char Arb_Char;
                    if (Arb_Value < 10)
                        Arb_Char = '0' + Arb_Value;
                    else 
                        Arb_Char = 'A' + Arb_Value - 10;
                    Text += string(" arb=\"") + Arb_Char + "\"";
                }
                if (Arb & (1 << 7)) // Repeat
                {
                    Text += (" arb_r=\"1\"");
                }
                if (Arb & (1 << 6)) // Non consecutive
                {
                    Text += (" arb_nc=\"1\"");
                }

                // Errors
                if (Frame->Errors)
                {
                    Text += ">\n";

                    // By DSeq
                    if (Frame->Video_STA_Errors)
                    {
                        size_t TotalPerSta[16];
                        memset(TotalPerSta, 0, 16 * sizeof(size_t));
                        for (auto Dseq4 = 0; Dseq4 < 16 * 16; Dseq4 += 16)
                        {
                            size_t Total = 0;
                            for (auto STA = 0; STA < 16; STA++)
                            {
                                auto Dseq4_STA = Dseq4 | STA;
                                Total += Frame->Video_STA_Errors[Dseq4_STA];
                                TotalPerSta[STA] += Frame->Video_STA_Errors[Dseq4_STA];
                            }
                            if (Total)
                            {
                                Text += "\t\t\t\t<dseq n=\"";
                                Text += to_string(Dseq4 >> 4);
                                Text += "\">\n";
                                for (auto STA = 0; STA < 16; STA++)
                                {
                                    auto n = Frame->Video_STA_Errors[Dseq4 | STA];
                                    if (n)
                                    {
                                        Text += "\t\t\t\t\t<error type=\"video error concealment ";
                                        Text += (STA < 10 ? '0' : ('A' - 10)) + STA;
                                        Text += "\" n=\"";
                                        Text += to_string(n);
                                        Text += "\"/>\n";
                                    }
                                }

                                Text += "\t\t\t\t</dseq>\n";
                            }
                        }
                        for (auto STA = 0; STA < 16; STA++)
                        {
                            auto n = TotalPerSta[STA];
                            if (n)
                            {
                                Text += "\t\t\t\t<error type=\"video error concealment ";
                                Text += (STA < 10 ? '0' : ('A' - 10)) + STA;
                                Text += "\" n=\"";
                                Text += to_string(n);
                                Text += "\"/>\n";
                            }
                        }
                    }

                    static const auto Errors_Text_Size = 6;
                    static const char* Errors_Text[] =
                    {
                        "video error concealment",
                        "audio error code",
                        "DV timecode incoherency",
                        "DIF incoherency",
                        "Arbitrary bit inconsistency",
                        "Stts fluctuation",
                    };
                    const auto Errors = Frame->Errors;
                    size_t c = 0, e = 0;
                    for (size_t i = 0; Errors[i]; i++)
                    {   
                        if (Errors[i] != '\t')
                            continue;

                        if (i != c)
                        {
                            Text += "\t\t\t\t<error type=\"";
                            if (e<Errors_Text_Size)
                                Text += Errors_Text[e];
                            else
                                Text += to_string(e);
                            Text += "\">";
                            Text.append(Errors + c, i - c);
                            Text += "</error>\n";
                        }

                        e++;
                        c = i + 1;
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

//***************************************************************************
// PerFile
//***************************************************************************

//---------------------------------------------------------------------------
void Core::PerFile_Clear()
{
    for (const auto& File : PerFile)
    {
        delete File;
    }
    PerFile.clear();
}
