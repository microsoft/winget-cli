// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionReporter.h"
#include <AppInstallerErrors.h>


namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;

    const Sequence& HelpCommandEmphasis = TextFormat::Foreground::Bright;
    const Sequence& HelpArgumentEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ManifestInfoEmphasis = TextFormat::Foreground::Bright;
    const Sequence& SourceInfoEmphasis = TextFormat::Foreground::Bright;
    const Sequence& NameEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& IdEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& UrlEmphasis = TextFormat::Foreground::BrightBlue;
    const Sequence& PromptEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ConvertToUpgradeFlowEmphasis = TextFormat::Foreground::BrightYellow;
    const Sequence& ConfigurationIntentEmphasis = TextFormat::Foreground::Bright;
    const Sequence& ConfigurationUnitEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& AuthenticationEmphasis = TextFormat::Foreground::BrightYellow;

    Reporter::Reporter(std::ostream& outStream, std::istream& inStream) :
        Reporter(std::make_shared<BaseStream>(outStream, true, ConsoleModeRestore::Instance().IsVTEnabled()), inStream)
    {
        SetProgressSink(this);
    }

    Reporter::Reporter(std::shared_ptr<BaseStream> outStream, std::istream& inStream) :
        m_out(outStream),
        m_in(inStream)
    {
        auto sixelSupported = [&]() { return SixelsSupported(); };
        m_spinner = IIndefiniteSpinner::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), VisualStyle::Accent, sixelSupported);
        m_progressBar = IProgressBar::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), VisualStyle::Accent, sixelSupported);

        SetProgressSink(this);
    }

    Reporter::~Reporter()
    {
        this->CloseOutputStream();
    }

    Reporter::Reporter(const Reporter& other, clone_t) :
        Reporter(other.m_out, other.m_in)
    {
        if (other.m_style.has_value())
        {
            SetStyle(*other.m_style);
        }
    }

    std::optional<PrimaryDeviceAttributes> Reporter::GetPrimaryDeviceAttributes()
    {
        if (ConsoleModeRestore::Instance().IsVTEnabled())
        {
            return PrimaryDeviceAttributes{ m_out->Get(), m_in };
        }
        else
        {
            return std::nullopt;
        }
    }

    OutputStream Reporter::GetOutputStream(Level level)
    {
        // If the level is not enabled, return a default stream which is disabled
        if (WI_AreAllFlagsClear(m_enabledLevels, level))
        {
            return OutputStream(*m_out, false, false);
        }

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
        return { *m_out, m_channel == Channel::Output };
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

        if (m_channel == Channel::Output)
        {
            auto sixelSupported = [&]() { return SixelsSupported(); };
            m_spinner = IIndefiniteSpinner::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), style, sixelSupported);
            m_progressBar = IProgressBar::CreateForStyle(*m_out, ConsoleModeRestore::Instance().IsVTEnabled(), style, sixelSupported);
        }

        if (style == VisualStyle::NoVT)
        {
            m_out->SetVTEnabled(false);
        }
    }

    bool Reporter::PromptForBoolResponse(Resource::LocString message, Level level, bool resultIfDisabled)
    {
        auto out = GetOutputStream(level);

        if (!out.IsEnabled())
        {
            return resultIfDisabled;
        }

        const std::vector<BoolPromptOption> options
        {
            BoolPromptOption{ Resource::String::PromptOptionYes, 'Y', true },
            BoolPromptOption{ Resource::String::PromptOptionNo, 'N', false },
        };

        out << message << std::endl;

        // Try prompting until we get a recognized option
        for (;;)
        {
            // Output all options
            for (size_t i = 0; i < options.size(); ++i)
            {
                out << PromptEmphasis << "[" + options[i].Hotkey.get() + "] " + options[i].Label.get();

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
                m_in.get();
                THROW_HR(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
            }

            // Find the matching option ignoring whitespace
            Utility::Trim(response);
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

    void Reporter::PromptForEnter(Level level)
    {
        auto out = GetOutputStream(level);
        if (!out.IsEnabled())
        {
            return;
        }

        out << std::endl << Resource::String::PressEnterToContinue << std::endl;
        m_in.get();
    }

    std::filesystem::path Reporter::PromptForPath(Resource::LocString message, Level level, std::filesystem::path resultIfDisabled)
    {
        auto out = GetOutputStream(level);

        if (!out.IsEnabled())
        {
            return resultIfDisabled;
        }

        // Try prompting until we get a valid answer
        for (;;)
        {
            out << message << ' ';

            // Read the response
            std::string response;
            if (!std::getline(m_in, response))
            {
                THROW_HR(APPINSTALLER_CLI_ERROR_PROMPT_INPUT_ERROR);
            }

            // Validate the path
            std::filesystem::path path{ response };
            if (path.is_absolute())
            {
                return path;
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

    void Reporter::SetProgressMessage(std::string_view message)
    {
        if (m_spinner)
        {
            m_spinner->SetMessage(message);
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
        SetProgressMessage({});
        GetBasicOutputStream() << VirtualTerminal::Cursor::Visibility::EnableShow;
    };

    Reporter::AsyncProgressScope::AsyncProgressScope(Reporter& reporter, IProgressSink* sink, bool hideProgressWhenDone) :
        m_reporter(reporter), m_callback(sink)
    {
        reporter.SetProgressCallback(&m_callback);
        sink->BeginProgress();
        m_hideProgressWhenDone = hideProgressWhenDone;
    }

    Reporter::AsyncProgressScope::~AsyncProgressScope()
    {
        m_reporter.get().SetProgressCallback(nullptr);
        m_callback.GetSink()->EndProgress(m_hideProgressWhenDone);
    }

    ProgressCallback& Reporter::AsyncProgressScope::Callback()
    {
        return m_callback;
    }

    IProgressCallback* Reporter::AsyncProgressScope::operator->()
    {
        return &m_callback;
    }

    bool Reporter::AsyncProgressScope::HideProgressWhenDone() const
    {
        return m_hideProgressWhenDone;
    }

    void Reporter::AsyncProgressScope::HideProgressWhenDone(bool value)
    {
        m_hideProgressWhenDone.store(value);
    }

    std::unique_ptr<Reporter::AsyncProgressScope> Reporter::BeginAsyncProgress(bool hideProgressWhenDone)
    {
        return std::make_unique<AsyncProgressScope>(*this, m_progressSink.load(), hideProgressWhenDone);
    }

    void Reporter::SetProgressCallback(ProgressCallback* callback)
    {
        auto lock = m_progressCallbackLock.lock_exclusive();
        // Attempting two progress operations at the same time; not supported.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_progressCallback != nullptr && callback != nullptr);
        m_progressCallback = callback;
    }

    void Reporter::CancelInProgressTask(bool force, CancelReason reason)
    {
        // TODO: Maybe ask the user if they really want to cancel?
        UNREFERENCED_PARAMETER(force);
        auto lock = m_progressCallbackLock.lock_shared();
        ProgressCallback* callback = m_progressCallback.load();
        if (callback)
        {
            if (!callback->IsCancelledBy(CancelReason::Any))
            {
                callback->SetProgressMessage(Resource::String::CancellingOperation());
                callback->Cancel(reason);
            }
        }
    }

    void Reporter::CloseOutputStream(bool forceDisable)
    {
        if (forceDisable)
        {
            m_out->Disable();
        }
        m_out->RestoreDefault();
    }

    void Reporter::SetLevelMask(Level reporterLevel, bool setEnabled) {

        if (setEnabled)
        {
            WI_SetAllFlags(m_enabledLevels, reporterLevel);
        }
        else
        {
            WI_ClearAllFlags(m_enabledLevels, reporterLevel);
        }
    }

    bool Reporter::SixelsSupported()
    {
        auto attributes = GetPrimaryDeviceAttributes();
        return (attributes ? attributes->Supports(PrimaryDeviceAttributes::Extension::Sixel) : false);
    }

    bool Reporter::SixelsEnabled()
    {
        return Settings::User().Get<Settings::Setting::EnableSixelDisplay>() && SixelsSupported();
    }
}
