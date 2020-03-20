// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionReporter.h"


namespace AppInstaller::CLI::Execution
{
    namespace details
    {
        void IndefiniteSpinner::ShowSpinner()
        {
            if (!m_spinnerJob.valid() && !m_spinnerRunning && !m_canceled)
            {
                m_spinnerRunning = true;
                m_spinnerJob = std::async(std::launch::async, &IndefiniteSpinner::ShowSpinnerInternal, this);
            }
        }

        void IndefiniteSpinner::StopSpinner()
        {
            if (!m_canceled && m_spinnerJob.valid() && m_spinnerRunning)
            {
                m_canceled = true;
                m_spinnerJob.get();
            }
        }

        void IndefiniteSpinner::ShowSpinnerInternal()
        {
            char spinnerChars[] = { '-', '\\', '|', '/' };

            // First wait for a small amount of time to enable a fast task to skip
            // showing anything, or a progress task to skip straight to progress.
            Sleep(100);

            for (int i = 0; !m_canceled; i++) {
                m_out << '\b' << spinnerChars[i] << std::flush;

                if (i == 3)
                {
                    i = -1;
                }

                Sleep(250);
            }

            m_out << '\b';
            m_canceled = false;
            m_spinnerRunning = false;
        }

        void ProgressBar::ShowProgress(bool running, uint64_t progress)
        {
            if (running)
            {
                if (m_isVisible)
                {
                    m_out << "\rProgress: " << progress;
                }
                else
                {
                    m_out << "Progress: " << progress;
                    m_isVisible = true;
                }
            }
            else
            {
                if (m_isVisible)
                {
                    m_out << std::endl;
                    m_isVisible = false;
                }
            }
        }
    }

    Reporter::OutputStream::OutputStream(std::ostream& out, bool enableVT) :
        m_out(out), m_isVTEnabled(enableVT) {}

    void Reporter::OutputStream::AddFormat(const VirtualTerminal::Sequence& sequence)
    {
        m_format.append(sequence.Get());
    }

    void Reporter::OutputStream::ApplyFormat()
    {
        if (m_isVTEnabled)
        {
            if (m_applyFormatAtOne)
            {
                if (!--m_applyFormatAtOne)
                {
                    m_out << m_format;
                }
            }
        }
    }

    Reporter::OutputStream& Reporter::OutputStream::operator<<(std::ostream& (__cdecl* f)(std::ostream&))
    {
        f(m_out);
        return *this;
    }

    Reporter::OutputStream& Reporter::OutputStream::operator<<(const VirtualTerminal::Sequence& sequence)
    {
        m_out << sequence;
        // Apply format after next output
        m_applyFormatAtOne = 2;
        return *this;
    }

    Reporter::~Reporter()
    {
        // The goal of this is to return output to its previous state.
        // For now, we assume this means "default".
        if (m_consoleMode.IsVTEnabled())
        {
            m_out << VirtualTerminal::TextFormat::Default;
        }
    }

    Reporter::OutputStream Reporter::GetOutputStream(Level level)
    {
        OutputStream result(m_out, m_consoleMode.IsVTEnabled());

        switch (level)
        {
        case Level::Verbose:
            result.AddFormat(VirtualTerminal::TextFormat::Default);
            break;
        case Level::Info:
            result.AddFormat(VirtualTerminal::TextFormat::Default);
            break;
        case Level::Warning:
            result.AddFormat(VirtualTerminal::TextFormat::Foreground::BrightYellow);
            break;
        case Level::Error:
            result.AddFormat(VirtualTerminal::TextFormat::Foreground::BrightRed);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    bool Reporter::PromptForBoolResponse(const std::string& msg, Level level)
    {
        UNREFERENCED_PARAMETER(level);

        m_out << msg << " (Y|N)" << std::endl;

        char response;
        m_in.get(response);

        return tolower(response) == 'y';
    }

    void Reporter::ShowMsg(const std::string& msg, Level level)
    {
        GetOutputStream(level) << msg << std::endl;
    }

    void Reporter::ShowProgress(bool running, uint64_t progress)
    {
        m_progressBar.ShowProgress(running, progress);
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
        UNREFERENCED_PARAMETER(type);
        ShowIndefiniteProgress(false);
        ShowProgress(true, (maximum ? static_cast<uint64_t>((static_cast<double>(current) / maximum) * 100) : current));
    }
}
