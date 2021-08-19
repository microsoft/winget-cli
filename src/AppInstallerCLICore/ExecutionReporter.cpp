// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionReporter.h"


namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;

    const Sequence& HelpCommandEmphasis = TextFormat::Foreground::Bright;
    const Sequence& HelpArgumentEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ManifestInfoEmphasis = TextFormat::Foreground::Bright;
    const Sequence& NameEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& IdEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& UrlEmphasis = TextFormat::Foreground::BrightBlue;
    const Sequence& PromptEmphasis = TextFormat::Foreground::Bright;

    Reporter::Reporter(std::ostream& outStream, std::istream& inStream) :
        m_out(outStream),
        m_in(inStream),
        m_progressBar(std::in_place, outStream, IsVTEnabled()),
        m_spinner(std::in_place, outStream, IsVTEnabled())
    {
        SetProgressSink(this);
    }

    Reporter::~Reporter()
    {
        // The goal of this is to return output to its previous state.
        // For now, we assume this means "default".
        GetBasicOutputStream() << TextFormat::Default;
    }

    Reporter::Reporter(const Reporter& other, clone_t) :
        Reporter(other.m_out, other.m_in)
    {
        if (other.m_style.has_value())
        {
            SetStyle(*other.m_style);
        }
    }

    OutputStream Reporter::GetOutputStream(Level level)
    {
        OutputStream result = GetBasicOutputStream();

        switch (level)
        {
        case Level::Verbose:
            result.AddFormat(TextFormat::Default);
            break;
        case Level::Info:
            result.AddFormat(TextFormat::Default);
            break;
        case Level::Warning:
            result.AddFormat(TextFormat::Foreground::BrightYellow);
            break;
        case Level::Error:
            result.AddFormat(TextFormat::Foreground::BrightRed);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    OutputStream Reporter::GetBasicOutputStream()
    {
        return { m_out, m_channel == Channel::Output, IsVTEnabled() };
    }

    void Reporter::SetChannel(Channel channel)
    {
        m_channel = channel;

        if (m_channel != Channel::Output)
        {
            // Disable progress for non-output channels
            m_spinner.reset();
            m_progressBar.reset();
        }
    }

    void Reporter::SetStyle(VisualStyle style)
    {
        m_style = style;
        if (m_spinner)
        {
            m_spinner->SetStyle(style);
        }
        if (m_progressBar)
        {
            m_progressBar->SetStyle(style);
        }
        if (style == VisualStyle::NoVT)
        {
            m_isVTEnabled = false;
        }
    }

    bool Reporter::PromptForBoolResponse(Resource::LocString message, Level level)
    {
        const std::vector<PromptOption> options
        {
            PromptOption{ Resource::String::PromptOptionYes, true },
            PromptOption{ Resource::String::PromptOptionNo, false },
        };

        auto out = GetOutputStream(level);
        out << message << std::endl;

        // Try prompting until we get a recognized option
        for (;;)
        {
            // Output all options
            for (size_t i = 0; i < options.size(); ++i)
            {
                out << PromptEmphasis << "[" + options[i].Hotkey + "] " + options[i].Label;

                if (i + 1 == options.size())
                {
                    out << PromptEmphasis << ": ";
                }
                else
                {
                    out << "  ";
                }
            }

            // Read the response
            std::string response;
            if (!std::getline(m_in, response))
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
            }

            // Ignore whitespace
            Utility::Trim(response);
            if (Utility::IsEmptyOrWhitespace(response))
            {
                continue;
            }

            // Find the matching option
            for (const auto& option : options)
            {
                if (Utility::CaseInsensitiveEquals(response, option.Label) ||
                    Utility::CaseInsensitiveEquals(response, option.Hotkey))
                {
                    return option.Value;
                }
            }
        }
    }

    void Reporter::ShowIndefiniteProgress(bool running)
    {
        if (m_spinner)
        {
            if (running)
            {
                m_spinner->ShowSpinner();
            }
            else
            {
                m_spinner->StopSpinner();
            }
        }
    }

    void Reporter::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        ShowIndefiniteProgress(false);
        if (m_progressBar)
        {
            m_progressBar->ShowProgress(current, maximum, type);
        }
    }
    
    void Reporter::BeginProgress()
    {
        GetBasicOutputStream() << VirtualTerminal::Cursor::Visibility::DisableShow;
        ShowIndefiniteProgress(true);
    };

    void Reporter::EndProgress(bool hideProgressWhenDone)
    {
        ShowIndefiniteProgress(false);
        if (m_progressBar)
        {
            m_progressBar->EndProgress(hideProgressWhenDone);
        }
        GetBasicOutputStream() << VirtualTerminal::Cursor::Visibility::EnableShow;
    };

    void Reporter::SetProgressCallback(ProgressCallback* callback)
    {
        auto lock = m_progressCallbackLock.lock_exclusive();
        m_progressCallback = callback;
    }

    void Reporter::CancelInProgressTask(bool force)
    {
        // TODO: Maybe ask the user if they really want to cancel?
        UNREFERENCED_PARAMETER(force);
        auto lock = m_progressCallbackLock.lock_shared();
        ProgressCallback* callback = m_progressCallback.load();
        if (callback)
        {
            callback->Cancel();
        }
    }

    bool Reporter::IsVTEnabled() const
    {
        return m_isVTEnabled && ConsoleModeRestore::Instance().IsVTEnabled();
    }

    PromptOption::PromptOption(Resource::StringId annotatedLabelId, bool value) : Value(value)
    {
        // The label we receive should be annotated with a '&' to indicate the hotkey
        auto annotatedLabel = Resource::LocString{ annotatedLabelId };

        auto pos = Utility::UTF8Find(annotatedLabel, HotkeyMarker);
        auto length = Utility::UTF8Length(annotatedLabel);

        if (pos != std::string::npos && pos < length - 1)
        {
            Hotkey = Utility::UTF8Substring(annotatedLabel, pos + 1, 1);
        }

        Label = std::string(Utility::UTF8Substring(annotatedLabel, 0, pos)) + std::string(Utility::UTF8Substring(annotatedLabel, pos + 1, length));
    }
}
