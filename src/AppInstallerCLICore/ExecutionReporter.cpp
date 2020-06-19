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
        m_consoleMode(),
        m_progressBar(outStream, m_consoleMode.IsVTEnabled()),
        m_spinner(outStream, m_consoleMode.IsVTEnabled())
    {}

    Reporter::OutputStream::OutputStream(std::ostream& out, bool enableVT) :
        m_out(out), m_isVTEnabled(enableVT) {}

    void Reporter::OutputStream::AddFormat(const Sequence& sequence)
    {
        m_format.append(sequence.Get());
    }

    void Reporter::OutputStream::ApplyFormat()
    {
        // Only apply format if m_applyFormatAtOne == 1 coming into this function.
        if (m_isVTEnabled && m_applyFormatAtOne)
        {
            if (!--m_applyFormatAtOne)
            {
                m_out << m_format;
            }
        }
    }

    Reporter::OutputStream& Reporter::OutputStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        f(m_out);
        return *this;
    }

    Reporter::OutputStream& Reporter::OutputStream::operator<<(const Sequence& sequence)
    {
        m_out << sequence;
        // An incoming sequence will be valid for 1 "standard" output after this one.
        // We set this to 2 to make that happen, because when it is 1, we will output
        // the format for the current OutputStream.
        m_applyFormatAtOne = 2;
        return *this;
    }

    Reporter::~Reporter()
    {
        // The goal of this is to return output to its previous state.
        // For now, we assume this means "default".
        if (m_consoleMode.IsVTEnabled())
        {
            m_out << TextFormat::Default;
        }
    }

    Reporter::OutputStream Reporter::GetOutputStream(Level level)
    {
        OutputStream result(m_out, m_consoleMode.IsVTEnabled());

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

    void Reporter::SetStyle(VisualStyle style)
    {
        m_spinner.SetStyle(style);
        m_progressBar.SetStyle(style);
        if (style == VisualStyle::NoVT)
        {
            m_consoleMode.DisableVT();
        }
    }

    bool Reporter::PromptForBoolResponse(const std::string& msg, Level level)
    {
        UNREFERENCED_PARAMETER(level);

        m_out << msg << " (Y|N)" << std::endl;

        char response;
        m_in.get(response);

        return tolower(response) == 'y';
    }

    void Reporter::ShowIndefiniteProgress(bool running)
    {
        if (running)
        {
            m_spinner.ShowSpinner();
        }
        else
        {
            m_spinner.StopSpinner();
        }
    }

    void Reporter::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        ShowIndefiniteProgress(false);
        m_progressBar.ShowProgress(current, maximum, type);
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
}
