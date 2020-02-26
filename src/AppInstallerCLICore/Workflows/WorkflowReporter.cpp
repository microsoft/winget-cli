// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow
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
            out << '\b' << spinnerChars[i] << std::flush;

            if (i == 3)
            {
                i = -1;
            }

            Sleep(250);
        }

        out << '\b';
        m_canceled = false;
        m_spinnerRunning = false;
    }

    void ProgressBar::ShowProgress(bool running, uint64_t progress)
    {
        if (running)
        {
            if (m_isVisible)
            {
                out << "\rProgress: " << progress;
            }
            else
            {
                out << "Progress: " << progress;
                m_isVisible = true;
            }
        }
        else
        {
            if (m_isVisible)
            {
                out << std::endl;
                m_isVisible = false;
            }
        }
    }

    bool WorkflowReporter::PromptForBoolResponse(Level level, const std::string& msg)
    {
        UNREFERENCED_PARAMETER(level);

        out << msg << " (Y|N)" << std::endl;

        char response;
        in.get(response);

        return tolower(response) == 'y';
    }

    void WorkflowReporter::ShowMsg(Level level, const std::string& msg)
    {
        UNREFERENCED_PARAMETER(level);

        // Todo: color output using level and possibly other factors.
        out << msg << std::endl;
    }

    void WorkflowReporter::ShowProgress(bool running, uint64_t progress)
    {
        m_progressBar.ShowProgress(running, progress);
    }

    void WorkflowReporter::ShowIndefiniteProgress(bool running)
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

    void WorkflowReporter::OnProgress(uint64_t current, uint64_t maximum, ProgressType type)
    {
        UNREFERENCED_PARAMETER(type);
        ShowIndefiniteProgress(false);
        ShowProgress(true, (maximum ? static_cast<uint64_t>((static_cast<double>(current) / maximum) * 100) : current));
    }
}
