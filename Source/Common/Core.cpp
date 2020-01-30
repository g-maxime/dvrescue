/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Common/Core.h"
#include "Common/ProcessFile.h"
#include "Common/Output_Xml.h"
#include "Common/Output_Webvtt.h"
#include "ZenLib/Ztring.h"
using namespace ZenLib;
//---------------------------------------------------------------------------

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
return_value Core::Process()
{
    // Analyze files
    PerFile_Clear();
    PerFile.reserve(Inputs.size());
    for (const auto& Input : Inputs)
        PerFile.push_back(new file(Input));

    // Set output defaults
    return_value ToReturn = ReturnValue_OK;
    if (!XmlFile)
        XmlFile = Out;

    // XML
    if (XmlFile)
    {
        *XmlFile << Output_Xml(PerFile);
        if (XmlFile->bad())
        {
            if (Err)
                *Err << "Error: can not write to XML output.\n";
            ToReturn = ReturnValue_ERROR;
        }
    }

    // WebVTT
    if (WebvttFile)
    {
        *WebvttFile << Output_Webvtt(PerFile);
        if (WebvttFile->bad())
        {
            if (Err)
                *Err << "Error: can not write to WebVTT output.\n";
            ToReturn = ReturnValue_ERROR;
        }
    }

    return ToReturn;
}

//---------------------------------------------------------------------------
float Core::State ()
{
    size_t Total = 0;
    for (const auto& File : PerFile)
        Total += File->MI.State_Get();
    return (((float)Total)/PerFile.size()/10000);
}

//***************************************************************************
// PerFile
//***************************************************************************

//---------------------------------------------------------------------------
void Core::PerFile_Clear()
{
    for (const auto& File : PerFile)
        delete File;
    PerFile.clear();
}
