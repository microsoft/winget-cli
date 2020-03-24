// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "VTSupport.h"


namespace AppInstaller::CLI::VirtualTerminal
{
    ConsoleModeRestore::ConsoleModeRestore(bool enableVTProcessing)
    {
        if (enableVTProcessing)
        {
            // Set output mode to handle virtual terminal sequences
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE)
            {
                LOG_LAST_ERROR();
            }
            else if (hOut == NULL)
            {
                AICLI_LOG(CLI, Info, << "VT not enabled due to null output handle");
            }
            else
            {
                if (!GetConsoleMode(hOut, &m_previousMode))
                {
                    LOG_LAST_ERROR();
                }
                else
                {
                    // Try to degrade in case DISABLE_NEWLINE_AUTO_RETURN isn't supported.
                    for (DWORD mode : { ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN, ENABLE_VIRTUAL_TERMINAL_PROCESSING})
                    {
                        DWORD outMode = m_previousMode | mode;
                        if (!SetConsoleMode(hOut, outMode))
                        {
                            // Even if it is a different error, log it and try to carry on.
                            LOG_LAST_ERROR_IF(GetLastError() != STATUS_INVALID_PARAMETER);
                        }
                        else
                        {
                            m_token = true;
                            m_isVTEnabled = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    ConsoleModeRestore::~ConsoleModeRestore()
    {
        if (m_token)
        {
            LOG_LAST_ERROR_IF(!SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), m_previousMode));
            m_token = false;
        }
    }

// The escape character that begins all VT sequences
#define AICLI_VT_ESCAPE     "\x1b"

// The beginning of a Control Sequence Introducer
#define AICLI_VT_CSI        AICLI_VT_ESCAPE "["

// The beginning of an Operating system command
#define AICLI_VT_OSC        AICLI_VT_ESCAPE "]"

    namespace TextFormat
    {
// Define a text formatting sequence with an integer id
#define AICLI_VT_TEXTFORMAT(_id_)       AICLI_VT_CSI #_id_ "m"

        Sequence Default = AICLI_VT_TEXTFORMAT(0);

        namespace Foreground
        {
            Sequence BrightRed = AICLI_VT_TEXTFORMAT(91);
            Sequence BrightYellow = AICLI_VT_TEXTFORMAT(93);
        }

        namespace Background
        {

        }
    };
}
