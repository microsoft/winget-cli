// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "VTSupport.h"
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>

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

        bool InitializeMode(DWORD handle, DWORD& previousMode, std::initializer_list<DWORD> modeModifierFallbacks, DWORD disabledFlags = 0)
        {
            HANDLE hStd = GetStdHandle(handle);
            if (hStd == INVALID_HANDLE_VALUE)
            {
                LOG_LAST_ERROR();
            }
            else if (hStd == NULL)
            {
                AICLI_LOG(CLI, Info, << "VT not enabled due to null handle [" << handle << "]");
            }
            else
            {
                if (!GetConsoleMode(hStd, &previousMode))
                {
                    // If the user redirects output, the handle will be invalid for this function.
                    // Don't log it in that case.
                    LOG_LAST_ERROR_IF(GetLastError() != ERROR_INVALID_HANDLE);
                }
                else
                {
                    for (DWORD mode : modeModifierFallbacks)
                    {
                        DWORD outMode = (previousMode & ~disabledFlags) | mode;
                        if (!SetConsoleMode(hStd, outMode))
                        {
                            // Even if it is a different error, log it and try to carry on.
                            LOG_LAST_ERROR_IF(GetLastError() != STATUS_INVALID_PARAMETER);
                        }
                        else
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        // Extracts a VT sequence, expected one of the form ESCAPE + prefix + result + suffix, returning the result part.
        std::string ExtractSequence(std::istream& inStream, std::string_view prefix, std::string_view suffix)
        {
            // Force discovery of available input
            std::ignore = inStream.peek();

            static constexpr std::streamsize s_bufferSize = 1024;
            char buffer[s_bufferSize];
            std::streamsize bytesRead = inStream.readsome(buffer, s_bufferSize);
            THROW_HR_IF(E_UNEXPECTED, bytesRead >= s_bufferSize);

            std::string_view resultView{ buffer, static_cast<size_t>(bytesRead) };
            size_t escapeIndex = resultView.find(AICLI_VT_ESCAPE[0]);
            if (escapeIndex == std::string_view::npos)
            {
                return {};
            }

            resultView = resultView.substr(escapeIndex);
            size_t overheadLength = 1 + prefix.length() + suffix.length();
            if (resultView.length() <= overheadLength ||
                resultView.substr(1, prefix.length()) != prefix ||
                resultView.substr(resultView.length() - suffix.length()) != suffix)
            {
                return {};
            }

            return std::string{ resultView.substr(1 + prefix.length(), resultView.length() - overheadLength) };
        }
    }

    ConsoleModeRestoreBase::ConsoleModeRestoreBase(DWORD handle) : m_handle(handle) {}

    ConsoleModeRestoreBase::~ConsoleModeRestoreBase()
    {
        if (m_token)
        {
            LOG_LAST_ERROR_IF(!SetConsoleMode(GetStdHandle(m_handle), m_previousMode));
            m_token = false;
        }
    }

    ConsoleModeRestore::ConsoleModeRestore() : ConsoleModeRestoreBase(STD_OUTPUT_HANDLE)
    {
        m_token = InitializeMode(STD_OUTPUT_HANDLE, m_previousMode, { ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN, ENABLE_VIRTUAL_TERMINAL_PROCESSING });
    }

    const ConsoleModeRestore& ConsoleModeRestore::Instance()
    {
        static ConsoleModeRestore s_instance;
        return s_instance;
    }

    ConsoleInputModeRestore::ConsoleInputModeRestore() : ConsoleModeRestoreBase(STD_INPUT_HANDLE)
    {
        m_token = InitializeMode(STD_INPUT_HANDLE, m_previousMode, { ENABLE_EXTENDED_FLAGS | ENABLE_VIRTUAL_TERMINAL_INPUT }, ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    }

    void ConstructedSequence::Append(const Sequence& sequence)
    {
        if (!sequence.Get().empty())
        {
            m_str += sequence.Get();
            Set(m_str);
        }
    }

    void ConstructedSequence::Clear()
    {
        m_str.clear();
        Set(m_str);
    }

// The beginning of a Control Sequence Introducer
#define AICLI_VT_CSI        AICLI_VT_ESCAPE "["

// The beginning of an Operating system command
#define AICLI_VT_OSC        AICLI_VT_ESCAPE "]"

    PrimaryDeviceAttributes::PrimaryDeviceAttributes(std::ostream& outStream, std::istream& inStream)
    {
        try
        {
            ConsoleInputModeRestore inputMode;
            if (!inputMode.IsVTEnabled())
            {
                return;
            }

            // Send DA1 Primary Device Attributes request
            outStream << AICLI_VT_CSI << "0c";
            outStream.flush();

            // Response is of the form AICLI_VT_CSI ? <conformance level> ; (<extension number> ;)* c
            std::string sequence = ExtractSequence(inStream, "[?", "c");
            std::vector<std::string> values = Utility::Split(sequence, ';');

            if (!values.empty())
            {
                m_conformanceLevel = std::stoul(values[0]);
            }

            for (size_t i = 1; i < values.size(); ++i)
            {
                m_extensions |= 1ull << std::stoul(values[i]);
            }
        }
        CATCH_LOG();
    }

    bool PrimaryDeviceAttributes::Supports(Extension extension) const
    {
        uint64_t extensionMask = 1ull << ToIntegral(extension);
        return (m_extensions & extensionMask) == extensionMask;
    }

    namespace Cursor
    {
        namespace Position
        {
            ConstructedSequence Up(int16_t cells)
            {
                THROW_HR_IF(E_INVALIDARG, cells < 0);
                std::ostringstream result;
                result << AICLI_VT_CSI << cells << 'A';
                return ConstructedSequence{ std::move(result).str() };
            }

            ConstructedSequence Down(int16_t cells)
            {
                THROW_HR_IF(E_INVALIDARG, cells < 0);
                std::ostringstream result;
                result << AICLI_VT_CSI << cells << 'B';
                return ConstructedSequence{ std::move(result).str() };
            }

            ConstructedSequence Forward(int16_t cells)
            {
                THROW_HR_IF(E_INVALIDARG, cells < 0);
                std::ostringstream result;
                result << AICLI_VT_CSI << cells << 'C';
                return ConstructedSequence{ std::move(result).str() };
            }

            ConstructedSequence Backward(int16_t cells)
            {
                THROW_HR_IF(E_INVALIDARG, cells < 0);
                std::ostringstream result;
                result << AICLI_VT_CSI << cells << 'D';
                return ConstructedSequence{ std::move(result).str() };
            }
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
            const Sequence Bright{ AICLI_VT_TEXTFORMAT(1) };
            const Sequence NoBright{ AICLI_VT_TEXTFORMAT(22) };

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
                return ConstructedSequence{ std::move(result).str() };
            }
        }

        namespace Background
        {
            ConstructedSequence Extended(const Color& color)
            {
                std::ostringstream result;
                result << AICLI_VT_CSI "48;2;" << static_cast<uint32_t>(color.R) << ';' << static_cast<uint32_t>(color.G) << ';' << static_cast<uint32_t>(color.B) << 'm';
                return ConstructedSequence{ std::move(result).str() };
            }
        }

        ConstructedSequence Hyperlink(const std::string& text, const std::string& ref)
        {
            std::ostringstream result;
            result << AICLI_VT_OSC "8;;" << ref << AICLI_VT_ESCAPE << "\\" << text << AICLI_VT_OSC << "8;;" << AICLI_VT_ESCAPE << "\\";
            return ConstructedSequence{ std::move(result).str() };
        }
    }

    namespace TextModification
    {
        const Sequence EraseLineForward{ AICLI_VT_CSI "0K" };
        const Sequence EraseLineBackward{ AICLI_VT_CSI "1K" };
        const Sequence EraseLineEntirely{ AICLI_VT_CSI "2K" };
    }

    namespace Progress
    {
        ConstructedSequence Construct(State state, std::optional<uint32_t> percentage)
        {
            // See https://conemu.github.io/en/AnsiEscapeCodes.html#ConEmu_specific_OSC

            THROW_HR_IF(E_BOUNDS, percentage.has_value() && percentage > 100u);

            // Workaround some quirks in the Windows Terminal implementation of the progress OSC sequence
            switch (state)
            {
            case State::None:
            case State::Indeterminate:
                // Windows Terminal does not recognize the OSC sequence if the progress value is left out.
                // As a workaround, we can specify an arbitrary value since it does not matter for None and Indeterminate states.
                percentage = percentage.value_or(0);
                break;
            case State::Normal:
            case State::Error:
            case State::Paused:
                // Windows Terminal does not support switching progress states without also setting a progress value at the same time,
                // so we disallow this case for now.
                THROW_HR_IF(E_INVALIDARG, !percentage.has_value());
                break;
            }

            int stateId;
            switch (state)
            {
            case State::None:
                stateId = 0;
                break;
            case State::Indeterminate:
                stateId = 3;
                break;
            case State::Normal:
                stateId = 1;
                break;
            case State::Error:
                stateId = 2;
                break;
            case State::Paused:
                stateId = 4;
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            std::ostringstream result;
            result << AICLI_VT_OSC "9;4;" << stateId << ";";
            if (percentage.has_value())
            {
                result << percentage.value();
            }
            result << AICLI_VT_ESCAPE << "\\";
            return ConstructedSequence{ std::move(result).str() };
        }
    }
}
