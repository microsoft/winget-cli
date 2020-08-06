// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionReporter.h"


namespace AppInstaller::CLI::Execution
{
    using namespace Settings;
    using namespace VirtualTerminal;

    const Sequence& HelpCommandEmphasis = TextFormat::Foreground::BrightWhite;
    const Sequence& HelpArgumentEmphasis = TextFormat::Foreground::BrightWhite;
    const Sequence& NameEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& IdEmphasis = TextFormat::Foreground::BrightCyan;
    const Sequence& UrlEmphasis = TextFormat::Foreground::BrightBlue;

    Reporter::Reporter(std::ostream& outStream, std::istream& inStream) :
        m_out(outStream),
        m_in(inStream),
        m_progressBar(std::in_place, outStream, IsVTEnabled()),
        m_spinner(std::in_place, outStream, IsVTEnabled())
    {}

    Reporter::~Reporter()
    {
        // The goal of this is to return output to its previous state.
        // For now, we assume this means "default".
        GetBasicOutputStream() << TextFormat::Default;
    }

    Reporter::Reporter(const Reporter& other, clone_t) :
        Reporter(other.m_out, other.m_in)
    {
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
}
