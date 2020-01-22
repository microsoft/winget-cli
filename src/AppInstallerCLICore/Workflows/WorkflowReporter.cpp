// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow
{
    void DownloaderCallback::OnStarted()
    {
        out << "Starting package download ..." << std::endl;
    }

    void DownloaderCallback::OnProgress(LONGLONG progress, LONGLONG downloadSize)
    {
        out << "\rDownloading " << progress << '/' << downloadSize;

        if (progress == downloadSize)
        {
            out << std::endl;
        }
    }

    void DownloaderCallback::OnCanceled()
    {
        out << "Package download canceled ..." << std::endl;
    }

    void DownloaderCallback::OnCompleted()
    {
        out << "Package download completed ..." << std::endl;
    }

    void WorkflowReporter::ShowPackageInfo(
        const std::string& name,
        const std::string& version,
        const std::string& author,
        const std::string& description,
        const std::string& homepage,
        const std::string& licenceUrl
    )
    {
        out << "Name: " << name << std::endl;
        out << "Version: " << version << std::endl;
        out << "Author: " << author << std::endl;
        out << "Description: " << description << std::endl;
        out << "Homepage: " << homepage << std::endl;
        out << "Licence: " << licenceUrl << std::endl;
    }

    char WorkflowReporter::GetCharResponse()
    {
        char response;
        in.get(response);
        return response;
    }

    void WorkflowReporter::ShowMsg(Level level, const std::string& msg)
    {
        // Todo: color output using level and possibly other factors.
        out << msg << std::endl;
    }

    void WorkflowReporter::ShowIndefiniteSpinner(bool running)
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

    void IndefiniteSpinner::ShowSpinner()
    {
        if (!m_spinnerJob.valid() && !m_spinnerRunning && !m_canceled)
        {
            m_spinnerRunning = true;
            m_spinnerJob = std::async(std::launch::async, &IndefiniteSpinner::ShowSpinnerInternal, this);
        }
    }

    void IndefiniteSpinner::ShowSpinnerInternal()
    {
        char spinnerChars[] = { '-', '\\', '|', '/' };

        for (int i = 0; !m_canceled; i++) {
            out << '\b' << spinnerChars[i] << std::flush;

            if (i == 3)
            {
                i = -1;
            }

            Sleep(300);
        }

        out << '\b';
        m_canceled = false;
        m_spinnerRunning = false;
    }

    void IndefiniteSpinner::StopSpinner()
    {
        if (!m_canceled && m_spinnerJob.valid() && m_spinnerRunning)
        {
            m_canceled = true;
            m_spinnerJob.wait();
        }
    }
}