// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Commands/Common.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void ShowFlow::Execute()
    {
        m_searchResult = WorkflowBase::IndexSearch();

        if (WorkflowBase::EnsureOneMatchFromSearchResult())
        {
            if (m_argsRef.Contains(CLI::ARG_LISTVERSIONS))
            {
                ShowAppVersion();
            }
            else
            {
                ShowAppInfo();
            }
        }
    }

    void ShowFlow::ShowAppInfo()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        auto manifest = app->GetManifest(
            m_argsRef.Contains(CLI::ARG_VERSION) ? *m_argsRef.GetArg(CLI::ARG_VERSION) : "",
            m_argsRef.Contains(CLI::ARG_CHANNEL) ? *m_argsRef.GetArg(CLI::ARG_CHANNEL) : ""
        );

        ManifestComparator manifestComparator(manifest, m_reporter);
        auto selectedLocalization = manifestComparator.GetPreferredLocalization(m_argsRef);
        auto selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);

        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Id: " + manifest.Id);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Name: " + manifest.Name);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Version: " + manifest.Version);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Author: " + manifest.Author);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "AppMoniker: " + manifest.AppMoniker);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Description: " + selectedLocalization.Description);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Homepage: " + selectedLocalization.Homepage);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "License: " + selectedLocalization.LicenseUrl);

        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Installer info:" + manifest.Id);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "--Installer Language: " + selectedInstaller.Language);
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "--Installer SHA256: " + Utility::SHA256::ConvertToString(selectedInstaller.Sha256));
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "--Installer Download Url: " + selectedInstaller.Url);
    }

    void ShowFlow::ShowAppVersion()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Id: " + app->GetId());
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Name: " + app->GetName());

        for (auto& version : app->GetVersions())
        {
            m_reporter.ShowMsg(WorkflowReporter::Level::Info, "  Version: " + version.first + ", Channel: " + version.second);
        }
    }
}