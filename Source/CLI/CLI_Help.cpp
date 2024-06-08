/*  Copyright (c) MIPoPS. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-3-Clause license that can
 *  be found in the LICENSE.txt file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "CLI/CLI_Help.h"
#include "Common/ProcessFile.h"
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
        "    --merge value | -m value\n"
        "        Merge all input files into value (file name),\n"
        "        picking the best part of each file.\n"
        "\n"
        "    --merge-log value\n"
        "        Store merge log to value (file name).\n"
        "\n"
        "    --merge-output-speed\n"
        "        Report and merge frames having speed not meaning normal playback\n"
        "        Is default (it will change in the future).\n"
        "\n"
        "    --merge-ignore-speed\n"
        "        Do not report and do not merge frames having speed not\n"
        "        meaning normal playback.\n"
        "\n"
        "    --merge-output-concealed\n"
        "        Report and merge frames having all blocks concealed.\n"
        "        Is default (it will change in the future).\n"
        "\n"
        "    --merge-ignore-concealed\n"
        "        Do not report and do not merge frames having all blocks concealed.\n"
        "\n"
        "    --merge-log-missing\n"
        "        Report frames considered as missing (due to time code jump etc).\n"
        "        Is default if information output format is not CSV.\n"
        "\n"
        "    --merge-hide-missing\n"
        "        Do not report frames considered as missing (due to time code jump etc).\n"
        "        Is default if information output format is CSV.\n"
        "\n"
        "    --merge-log-intermediate\n"
        "        Display additional lines of information\n"
        "        about intermediate analysis during files merge.\n"
        "        Is default if information output format is not CSV.\n"
        "\n"
        "    --merge-hide-intermediate\n"
        "        Hide additional lines of information\n"
        "        about intermediate analysis during files merge.\n"
        "        Is default if information output format is CSV.\n"
        "\n"
        "    --csv\n"
        "        Output is in CSV format rather than human readable text.\n"
        "\n"
        "    --caption-presence-change value\n"
        "        Split XML \"frames\" elements when there is a caption presence\n"
        "        change (value = \"y\") or do not split (value = \"n\").\n"
        "        Default is \"n\".\n"
        "\n"
        "    --verbosity value | -v value\n"
        "        Verbosity of the output set to value:\n"
        "        0: no output.\n"
        "        5: summary only.\n"
        "        7: information per frame if there is a problem + summary.\n"
        "        9: information per frame + summary.\n"
        "\n"
        "    --timeout value\n"
        "        Time out limit for the device or pipe input (\"-\" file name) set to value (in seconds)\n"
        "\n"
        "    --capture\n"
        "        Launch capture.\n"
        "        Is the default if no --cmd option is provided.\n"
        "        Usable only if input is a device.\n"
        "\n"
        #if defined(ENABLE_SONY9PIN) || defined(ENABLE_DECKLINK)
        "    --control <port>\n"
        "        Set the serial port used to control the DeckLink capture device through sony9pin interface.\n"
        "        Use \"native\" to request control through DeckLink serial inteface.\n"
        "\n"
        #endif
        "    --in-control\n"
        "        Include an integrated command line input for controlling the input.\n"
        "        Usable only if input is a device.\n"
        "\n"
        "    --list_devices\n"
        "        List detected devices and their ID.\n"
        "\n"
        "    --list_devices_json\n"
        "        List detected devices and their ID (JSON output).\n"
        "\n"
        #ifdef ENABLE_SONY9PIN
        "    --list_controls\n"
        "        List detected serial ports.\n"
        "\n"
        "    --list_controls_json\n"
        "        List detected serial ports (JSON output).\n"
        "\n"
        #endif
        "    --status\n"
        "        Provide the status (playing, stop...) of the input.\n"
        "        By default device://0 is used.\n"
        "        Usable only if input is a device.\n"
        "\n"
        "    --cmd value\n"
        "        Send a command to the input.\n"
        "        By default device://0 is used.\n"
        "        Usable only if input is a device.\n"
        "        value may be:\n"
        "        play      Set speed to 1.0 and mode to play.\n"
        "        srew      Set speed to -1.0 and mode to play.\n"
        "        stop      Set speed to 0.0 and mode to no-play.\n"
        "        rew       Set speed to -2.0 and mode to play.\n"
        "        ff        Set speed to 2.0 and mode to play.\n"
        "\n"
        "    --mode value\n"
        "        Send a command to the input with the specified mode.\n"
        "        By default device://0 is used.\n"
        "        By default value is n if speed is 0 else p.\n"
        "        Usable only if input is a device.\n"
        "        value may be:\n"
        "        n         Set mode to no-play.\n"
        "        p         Set mode to play.\n"
        "\n"
        "    --speed value\n"
        "        Send a command to the input with the specified speed (float).\n"
        "        By default device://0 is used.\n"
        "        By default value is 0 if mode is no-play else 1.\n"
        "        Usable only if input is a device.\n"
        "\n"
        "    --rewind-count value\n"
        "        Automatically rewind to last good frame and capture again,\n"
        "        value times.\n"
        "        Usable only if input is a device.\n"
        "\n"
        "    --rewind\n"
        "        Same as --rewind-count 1\n"
        "\n"
        "    --rewind-basename value\n"
        "        Base name of files storing buggy frames per take\n"
        "        Default is output file name.\n"
        "\n"
        #ifdef ENABLE_DECKLINK
        "    --decklink-video-mode <mode>\n"
        "        Select DeckLink video mode.\n"
        "        value may be:\n"
        "        ntsc      Set video mode to NTSC (default).\n"
        "        pal       Set video mode to PAL.\n"
        "\n"
        "    --decklink-video-source <source>\n"
        "        Select DeckLink video source.\n"
        "        value may be:\n"
        "        sdi        Set video source to SDI (default).\n"
        "        hdmi       Set video source to HDMI.\n"
        "        optical    Set video source to optical SDI.\n"
        "        component  Set video source to component input.\n"
        "        composite  Set video source to composite input.\n"
        "        s_video    Set video source to S-Video input.\n"
        "\n"
        "    --decklink-audio-source <source>\n"
        "        Select DeckLink audio source.\n"
        "        value may be:\n"
        "        embedded    Set audio source to embedded input (default).\n"
        "        aes_ebu     Set audio source to AES-EBU input.\n"
        "        analog      Set audio source to analog input.\n"
        "        analog_xlr  Set audio source to analog(XLR) input.\n"
        "        analog_rca  Set audio source to analog(RCA) input.\n"
        "        microphone  Set audio source to microphone input.\n"
        "\n"
        "    --decklink-pixel-format <format>\n"
        "        Select DeckLink pixel format.\n"
        "        value may be:\n"
        "        uyvy        Set pixel format to 8-bit YUV as packed UYVY.\n"
        "        v210        Set pixel format to 10-bit  YUV as packed v210 (default).\n"
        "\n"
        "    --decklink-timecode-format <format>\n"
        "        Select DeckLink timecode format.\n"
        "        value may be:\n"
        "        none        Disable timecode capture.\n"
        "        rp188vitc   Set timecode format to RP188(VITC1).\n"
        "        rp188vitc2  Set timecode format to RP188(VITC2).\n"
        "        rp188ltc    Set timecode format to RP188(LTC).\n"
        "        rp188hfr    Set timecode format to RP188(HFR).\n"
        "        rp188any    Set timecode format to RP188(first valid RP188 format).\n"
        "        vitc        Set timecode format to VITC (default).\n"
        "        vitc2       Set timecode format to VITC Field 2.\n"
        "        serial      Set timecode source to serial input.\n"
        "\n"
        #endif
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
string NameVersion_Text()
{
    return
        "DVRescue v." Program_Version
        " (MediaInfoLib v." + MediaInfo_Version() + ")"
        " by MIPoPS"
        ;
}
