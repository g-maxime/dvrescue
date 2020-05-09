/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Common/ProcessFile.h"
#include "ZenLib/Ztring.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

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
        case MediaInfo_Event_DvDif_Analysis_Frame: if (EventVersion == 1 && Data_Size >= sizeof(struct MediaInfo_Event_DvDif_Analysis_Frame_1)) UserHandler->AddFrame((MediaInfo_Event_DvDif_Analysis_Frame_1*)Event_Generic); break;
        case MediaInfo_Event_DvDif_Change: if (EventVersion == 0 && Data_Size >= sizeof(struct MediaInfo_Event_DvDif_Change_0)) UserHandler->AddChange((MediaInfo_Event_DvDif_Change_0*)Event_Generic); break;
        case MediaInfo_Event_Global_Demux: if (EventVersion == 4 && Data_Size >= sizeof(struct MediaInfo_Event_Global_Demux_4)) UserHandler->AddFrame((MediaInfo_Event_Global_Demux_4*)Event_Generic); break;
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
    FrameNumber = 0;

    MI.Option(__T("File_Event_CallBackFunction"), __T("CallBack=memory://") + Ztring::ToZtring((size_t)&Event_CallBackFunction) + __T(";UserHandler=memory://") + Ztring::ToZtring((size_t)this));
    MI.Option(__T("File_DvDif_Analysis"), __T("1"));
    MI.Open(FileName);

    // Filing some info
    FrameRate = Ztring(MI.Get(Stream_Video, 0, __T("FrameRate_Original"))).To_float64();
    if (!FrameRate)
        FrameRate = Ztring(MI.Get(Stream_Video, 0, __T("FrameRate"))).To_float64();
    if (!FrameRate || (FrameRate >= 29.97 && FrameRate <= 29.98))
        FrameRate = double(30 / 1.001); // Default if no frame rate available, or better rounding
}

//---------------------------------------------------------------------------
file::~file()
{
    for (auto& Frame : PerFrame)
    {
        delete[] Frame->Errors;
        delete[] Frame->Video_STA_Errors;
        delete[] Frame->Audio_Data_Errors;
        delete Frame;
    }
    for (auto& Change : PerChange)
    {
        delete Change;
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
            && Current->VideoChromaSubsampling == FrameData->VideoChromaSubsampling
            && Current->VideoScanType == FrameData->VideoScanType
            && Current->VideoRatio_N == FrameData->VideoRatio_N
            && Current->VideoRatio_D == FrameData->VideoRatio_D
            && Current->VideoRate_N == FrameData->VideoRate_N
            && Current->VideoRate_D == FrameData->VideoRate_D
            && Current->AudioRate_N == FrameData->AudioRate_N
            && Current->AudioRate_D == FrameData->AudioRate_D
            && Current->AudioChannels == FrameData->AudioChannels
            && Current->AudioBitDepth == FrameData->AudioBitDepth)
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
void file::AddFrame(const MediaInfo_Event_DvDif_Analysis_Frame_1* FrameData)
{
    MediaInfo_Event_DvDif_Analysis_Frame_1* ToPush = new MediaInfo_Event_DvDif_Analysis_Frame_1();
    std::memcpy(ToPush, FrameData, sizeof(MediaInfo_Event_DvDif_Analysis_Frame_1));
    if (FrameData->Errors)
    {
        size_t SizeToCopy = std::strlen(FrameData->Errors) + 1;
        auto Errors = new char[SizeToCopy];
        std::memcpy(Errors, FrameData->Errors, SizeToCopy);
        ToPush->Errors = Errors;
    }
    if (FrameData->Video_STA_Errors)
    {
        size_t SizeToCopy = FrameData->Video_STA_Errors_Count * sizeof(size_t);
        auto Video_STA_Errors = new size_t[SizeToCopy];
        std::memcpy(Video_STA_Errors, FrameData->Video_STA_Errors, SizeToCopy);
        ToPush->Video_STA_Errors = Video_STA_Errors;
    }
    if (FrameData->Audio_Data_Errors)
    {
        size_t SizeToCopy = FrameData->Audio_Data_Errors_Count * sizeof(size_t);
        auto Audio_Data_Errors = new size_t[SizeToCopy];
        std::memcpy(Audio_Data_Errors, FrameData->Audio_Data_Errors, SizeToCopy);
        ToPush->Audio_Data_Errors = Audio_Data_Errors;
    }
    PerFrame.push_back(ToPush);
}

//---------------------------------------------------------------------------
void file::AddFrame(const MediaInfo_Event_Global_Demux_4* FrameData)
{
    if (!FrameData->StreamIDs_Size)
        return;
    auto Dseq = FrameData->StreamIDs[FrameData->StreamIDs_Size-1];
    if ((Dseq & 0xFFFF00) != ((0x2 << 16) | (0x65 << 8))) // identifier with SCT = 0x2, PackType = 0x65, and Dseq
        return;
    Dseq &= 0xFF; // Extract Dseq
    if (Dseq >= 16)
        return;
    if (FrameData->Content_Size != 4)
        return;
    
    // Store
    for (int i = 0; i < 2; i++)
    {
        // Filter unneeded content
        if (FrameData->Content[i * 2] == 0x80 && FrameData->Content[i * 2 + 1] == 0x80)
            return; // Don't store empty content
        if (Dseq && PerFrame_Captions_PerSeq_PerField.size() == 1)
        {
            auto& PerSeq0 = PerFrame_Captions_PerSeq_PerField[0];
            auto& FieldData0 = PerSeq0.FieldData[i];
            if (!FieldData0.empty() && FieldData0.back().StartFrameNumber + FieldData0.back().Captions.size() - 1 == FrameNumber && !FieldData0.back().Captions.back().compare(FrameData->Content + i * 2))
                continue; // Content is same, not stored
        }

        // Store
        if (Dseq >= PerFrame_Captions_PerSeq_PerField.size())
            PerFrame_Captions_PerSeq_PerField.resize(Dseq + 1);
        auto& PerSeq = PerFrame_Captions_PerSeq_PerField[Dseq];
        auto& FieldData = PerSeq.FieldData[i];
        if (FieldData.empty() || FieldData.back().StartFrameNumber + FieldData.back().Captions.size() != FrameNumber)
            FieldData.emplace_back(FrameNumber);
        FieldData.back().Captions.emplace_back(FrameData->Content + i * 2);
    }
}
