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
    std::vector<MediaInfo_Event_DvDif_Change_0*> PerChange;
    std::vector<MediaInfo_Event_DvDif_Analysis_Frame_0*> PerFrame;
    double FrameRate;
    size_t FrameNumber;

    file(const String& FileName);
    ~file();

    void AddChange(const MediaInfo_Event_DvDif_Change_0* FrameData);
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
        case MediaInfo_Event_DvDif_Analysis_Frame: if (EventVersion == 0 && Data_Size >= sizeof(struct MediaInfo_Event_DvDif_Analysis_Frame_0)) UserHandler->AddFrame((MediaInfo_Event_DvDif_Analysis_Frame_0*)Event_Generic); break;
        case MediaInfo_Event_DvDif_Change: if (EventVersion == 0 && Data_Size >= sizeof(struct MediaInfo_Event_DvDif_Change_0)) UserHandler->AddChange((MediaInfo_Event_DvDif_Change_0*)Event_Generic); break;
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
    FrameRate = Ztring(MI.Get(Stream_Video, 0, __T("FrameRate_Original"))).To_float64();
    if (!FrameRate)
        FrameRate = Ztring(MI.Get(Stream_Video, 0, __T("FrameRate"))).To_float64();
    if (!FrameRate || (FrameRate >= 29.97 && FrameRate <= 29.98))
        FrameRate = double(30 / 1.001); // Default if no frame rate available, or better rounding
    FrameNumber = 0;
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
void file::AddChange(const MediaInfo_Event_DvDif_Change_0* FrameData)
{
    FrameNumber++; // Event FrameCount is currently wrong

    // Check if there is a change we support
    if (!PerChange.empty())
    {
        const auto Current = PerChange.back();
        if (Current->Width == FrameData->Width
            && Current->Height == FrameData->Height
            && Current->VideoRatio_N == FrameData->VideoRatio_N
            && Current->VideoRatio_D == FrameData->VideoRatio_D
            && Current->VideoRate_N == FrameData->VideoRate_N
            && Current->VideoRate_D == FrameData->VideoRate_D
            && Current->AudioRate_N == FrameData->AudioRate_N
            && Current->AudioRate_D == FrameData->AudioRate_D
            && Current->AudioChannels == FrameData->AudioChannels)
        {
            return;
        }
    }

    MediaInfo_Event_DvDif_Change_0* ToPush = new MediaInfo_Event_DvDif_Change_0();
    std::memcpy(ToPush, FrameData, sizeof(MediaInfo_Event_DvDif_Change_0));
    ToPush->FrameNumber = FrameNumber - 1; // Event FrameCount is currently wrong
    PerChange.push_back(ToPush);
}

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
        size_t SizeToCopy = 8 * 16 * 16 * sizeof(size_t);
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
string to_timestamp(double Seconds_Float)
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

//---------------------------------------------------------------------------
char to_hex4(int Value)
{
    if (Value >= 16)
        return 'X';
    if (Value >= 10)
        return 'A' - 10 + Value;
    return '0' + Value;
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
        if (File->PerChange.empty())
        {
            // Handling old MediaInfo not having this feature
            MediaInfo_Event_DvDif_Change_0* ToPush = new MediaInfo_Event_DvDif_Change_0();
            std::memset(ToPush, 0x00, sizeof(MediaInfo_Event_DvDif_Change_0)); // TODO: real data from MediaInfo::Get()
            File->PerChange.push_back(ToPush);
        }

        // Media header
        Text += "\t<media ref=\"" + Ztring(File->MI.Get(Stream_General, 0, __T("CompleteName"))).To_UTF8() + "\">\n";

        // By Frame - For each line
        auto FrameNumber_Max = File->PerFrame.size() - 1;
        auto PerChange_Next = File->PerChange.begin();
        auto ShowFrames = true;
        for (const auto& Frame : File->PerFrame)
        {
            decltype(FrameNumber_Max) FrameNumber = &Frame - &*File->PerFrame.begin();
            auto ShowFrame = ShowFrames || Frame->Video_STA_Errors || FrameNumber == FrameNumber_Max;

            if (ShowFrames)
            {
                ShowFrames = false;

                if (FrameNumber)
                    Text += "\t\t</frames>\n";

                auto Change = *PerChange_Next;
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
                    Text += to_timestamp(TimeStamp_Begin);
                    Text += '\"';
                }
                {
                    auto TimeStamp_End = (PerChange_Next != File->PerChange.end() ? (*PerChange_Next)->FrameNumber : (FrameNumber_Max + 1)) / File->FrameRate;
                    Text += " end_pts=\"";
                    Text += to_timestamp(TimeStamp_End);
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
                    Text += " audio_channels=\"";
                    Text += to_string(Change->AudioChannels);
                    Text += '\"';
                }
                Text += ">\n";
            }

            if (!ShowFrame)
            {
                if (PerChange_Next != File->PerChange.end() && (*PerChange_Next)->FrameNumber == FrameNumber + 1)
                {
                    ShowFrame = true;
                    ShowFrames = true; // For next frame
                }
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
                    Text += to_timestamp(TimeStamp);
                    Text += '\"';
                }

                // TimeCode
                auto TimeCode_Seconds = (Frame->TimeCode >> 8) & 0x7FFFF; // Value
                if (TimeCode_Seconds != 0x7FFFF)
                {
                    auto TimeCode_DropFrame = Frame->TimeCode & 0x00000080 ? true : false;
                    auto TimeCode_Frames = Frame->TimeCode & 0x3F;
                    Text += " tc=\"" + TimeCode2String(TimeCode_Seconds, TimeCode_DropFrame, TimeCode_Frames) + "\"";
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
                if (Frame->Video_STA_Errors) //Frame->Errors)
                {
                    Text += ">\n";

                    // Split
                    if (Frame->Video_STA_Errors)
                    {
                        size_t TotalPerSta[16];
                        memset(TotalPerSta, 0, 16 * sizeof(size_t));
                        for (auto Dseq7 = 0; Dseq7 < 16 * 128; Dseq7 += 128)
                        {
                            size_t TotalPerDseqPerSta[16];
                            memset(TotalPerDseqPerSta, 0, 16 * sizeof(size_t));
                            auto Dseq_Open = false;
                            for (auto Sct4 = 0; Sct4 < 8 * 16; Sct4 += 16)
                            {
                                auto Sct_Open = false;
                                size_t Total = 0;
                                for (auto STA = 0; STA < 16; STA++)
                                {
                                    auto Dseq7_Sct4_STA = Dseq7 | Sct4 | STA;
                                    auto n = Frame->Video_STA_Errors[Dseq7_Sct4_STA];
                                    Total += n;
                                    TotalPerDseqPerSta[STA] += n;
                                    TotalPerSta[STA] += n;
                                }
                                if (Total)
                                {
                                    if (!Dseq_Open)
                                    {
                                        Text += "\t\t\t\t<dseq n=\"";
                                        Text += to_string(Dseq7 >> 7);
                                        Text += "\">\n";
                                        Dseq_Open = true;
                                    }
                                    if (!Sct_Open)
                                    {
                                        Text += "\t\t\t\t\t<sct t=\"";
                                        Text += to_string(Sct4 >> 4);
                                        Text += "\">\n";
                                        Sct_Open = true;
                                    }
                                    for (auto STA = 0; STA < 16; STA++)
                                    {
                                        auto Dseq7_Sct4_STA = Dseq7 | Sct4 | STA;
                                        auto n = Frame->Video_STA_Errors[Dseq7_Sct4_STA];
                                        if (n)
                                        {
                                            Text += "\t\t\t\t\t\t<sta t=\"";
                                            Text += to_string(STA);
                                            Text += "\" n=\"";
                                            Text += to_string(n);
                                            Text += "\"/>\n";
                                        }
                                    }

                                    if (Sct_Open)
                                    {
                                        Text += "\t\t\t\t\t</sct>\n";
                                    }
                                }
                            }
                            if (Dseq_Open)
                            {
                                for (auto STA = 0; STA < 16; STA++)
                                {
                                    auto n = TotalPerDseqPerSta[STA];
                                    if (n)
                                    {
                                        Text += "\t\t\t\t\t<sta t=\"";
                                        Text += to_string(STA);
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
                                Text += "\t\t\t\t<sta t=\"";
                                Text += to_string(STA);
                                Text += "\" n=\"";
                                Text += to_string(n);
                                Text += "\"/>\n";
                            }
                        }
                    }

                    /*
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
                    */

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
