// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow
{
    void WorkflowReporter::ShowAppInfo(
        const AppInstaller::Manifest::Manifest& manifest,
        const AppInstaller::Manifest::ManifestLocalization& selectedLocalization,
        const AppInstaller::Manifest::ManifestInstaller& selectedInstaller)
    {
        out << "Id: " << manifest.Id << std::endl;
        out << "Name: " << manifest.Name << std::endl;
        out << "Version: " << manifest.Version << std::endl;
        out << "Author: " << manifest.Author << std::endl;
        out << "AppMoniker: " << manifest.AppMoniker << std::endl;
        out << "Description: " << selectedLocalization.Description << std::endl;
        out << "Homepage: " << selectedLocalization.Homepage << std::endl;
        out << "License: " << selectedLocalization.LicenseUrl << std::endl;

        out << "Installer info:" << manifest.Id << std::endl;
        out << "--Installer Type: " << selectedInstaller.InstallerType << std::endl;
        out << "--Installer Language: " << selectedInstaller.Language << std::endl;
        out << "--Installer SHA256: " << Utility::SHA256::ConvertToString(selectedInstaller.Sha256) << std::endl;
        out << "--Installer Download Url: " << selectedInstaller.Url << std::endl;
    }

    void WorkflowReporter::ShowSearchResult(const AppInstaller::Repository::SearchResult& result)
    {
        for (auto& match : result.Matches)
        {
            auto app = match.Application.get();
            out << "App Id: " << app->GetId() << std::endl;
            out << "App Name: " << app->GetName() << std::endl;
            out << "Available Versions:" << std::endl;

            for (auto& version : app->GetVersions())
            {
                out << "  Version: " << version.first << ", Channel: " << version.second << std::endl;
            }

            out << std::endl;
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

    void WorkflowReporter::ShowProgress(bool running, int progress)
    {
        m_progressBar.ShowProgress(running, progress);
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

    void ProgressBar::ShowProgress(bool running, int progress)
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
}