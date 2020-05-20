// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "VTSupport.h"


namespace AppInstaller::CLI::VirtualTerminal
{
    namespace
    {
        TextFormat::Color GetAccentColorFromSystem()
        {
            using namespace winrt::Windows::UI::ViewManagement;

            UISettings settings;
            auto color = settings.GetColorValue(UIColorType::Accent);
            return { color.R, color.G, color.B };
        }
    }

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
                    // If the user redirects output, the handle will be invalid for this function.
                    // Don't log it in that case.
                    LOG_LAST_ERROR_IF(GetLastError() != ERROR_INVALID_HANDLE);
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

    namespace Cursor
    {
        namespace Position
        {
#define AICLI_VT_SIMPLE_CURSORPOSITON(_c_) AICLI_VT_ESCAPE #_c_

            const Sequence UpOne{ AICLI_VT_SIMPLE_CURSORPOSITON(A) };
            const Sequence DownOne{ AICLI_VT_SIMPLE_CURSORPOSITON(B) };
            const Sequence ForwardOne{ AICLI_VT_SIMPLE_CURSORPOSITON(C) };
            const Sequence BackwardOne{ AICLI_VT_SIMPLE_CURSORPOSITON(D) };
        }

        namespace Visibility
        {
            const Sequence EnableBlink{ AICLI_VT_CSI "?12h" };
            const Sequence DisableBlink{ AICLI_VT_CSI "?12l" };
            const Sequence EnableShow{ AICLI_VT_CSI "?25h" };
            const Sequence DisableShow{ AICLI_VT_CSI "?25l" };
        }
    }

    namespace TextFormat
    {
// Define a text formatting sequence with an integer id
#define AICLI_VT_TEXTFORMAT(_id_)       AICLI_VT_CSI #_id_ "m"

        const Sequence Default{ AICLI_VT_TEXTFORMAT(0) };
        const Sequence Negative{ AICLI_VT_TEXTFORMAT(7) };

        Color Color::GetAccentColor()
        {
            static Color accent{ GetAccentColorFromSystem() };
            return accent;
        }

        namespace Foreground
        {
            const Sequence BrightRed{ AICLI_VT_TEXTFORMAT(91) };
            const Sequence BrightGreen{ AICLI_VT_TEXTFORMAT(92) };
            const Sequence BrightYellow{ AICLI_VT_TEXTFORMAT(93) };
            const Sequence BrightBlue{ AICLI_VT_TEXTFORMAT(94) };
            const Sequence BrightMagenta{ AICLI_VT_TEXTFORMAT(95) };
            const Sequence BrightCyan{ AICLI_VT_TEXTFORMAT(96) };
            const Sequence BrightWhite{ AICLI_VT_TEXTFORMAT(97) };

            ConstructedSequence Extended(const Color& color)
            {
                std::ostringstream result;
                result << AICLI_VT_CSI "38;2;" << static_cast<uint32_t>(color.R) << ';' << static_cast<uint32_t>(color.G) << ';' << static_cast<uint32_t>(color.B) << 'm';
                return ConstructedSequence{ result.str() };
            }
        }

        namespace Background
        {

        }
    }

    namespace TextModification
    {
        const Sequence EraseLineForward{ AICLI_VT_CSI "0K" };
        const Sequence EraseLineBackward{ AICLI_VT_CSI "1K" };
        const Sequence EraseLineEntirely{ AICLI_VT_CSI "2K" };
    }
}
