/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "CLI/CLI_Help.h"
#include <ostream>
using namespace std;
//---------------------------------------------------------------------------

//***************************************************************************
// Help
//***************************************************************************

//---------------------------------------------------------------------------
return_value Help(ostream& Out, const char* Name, bool Full)
{
    Out <<
    "Usage: \"" << Name << " FileName1 [Filename2...] [Options...]\"\n";
    if (!Full)
    {
        Out << "\"" << Name << " --help\" for displaying more information.\n"
            << endl;
        return ReturnValue_OK;
    }
    Out << "\n"
        "Options:\n"
        "    --help, -h\n"
        "        Display this help and exit.\n"
        "\n"
        "    --version\n"
        "        Display DVRescue version and exit.\n"
        "\n"
        "    --cc-format value\n"
        "        Set Closed Captions output format to value.\n"
        "        value can be 'scc', 'screen', 'srt'.\n"
        "        If there is more than one instance of this option,\n"
        "        this option is applied to the next --cc-output option.\n"
        "\n"
        "    --cc-output value | -c value\n"
        "        Store Closed Captions output to value (file name).\n"
        "        File extension must be the format name (see above)\n"
        "        if --cc-format is not provided.\n"
        "        if content is different between Dseq and/or has more than 1 field,\n"
        "        extension is prefixed by 'dseq%dseq%.' and/or 'field%field%.'.\n"
        "        There can be more than one instance of this option.\n"
        "\n"
        "    --cc-tc value\n"
        "        Set Closed Captions output start time code to value.\n"
        "        Used for SCC output.\n"
        "        value format is HH:MM:SS;FF, or 'dv' (for DV first frame time code).\n"
        "\n"
        "    --webvtt-output value | -s value\n"
        "        Store WebVTT output to value (file name).\n"
        "\n"
        "    --xml-output value | -x value\n"
        "        Store XML output to value (file name).\n"
        "\n"
        "If no output file name is provided, XML output is displayed on console output."
        "\n"
        << endl;
    
    return ReturnValue_OK;
}

//---------------------------------------------------------------------------
return_value NameVersion(ostream& Out)
{
    Out <<
    NameVersion_Text() << ".\n"
    ;

    return ReturnValue_OK;
}

//***************************************************************************
// Info
//***************************************************************************

//---------------------------------------------------------------------------
const char* NameVersion_Text()
{
    return
        "DVRescue v." Program_Version " by MIPoPS"
        ;
}
