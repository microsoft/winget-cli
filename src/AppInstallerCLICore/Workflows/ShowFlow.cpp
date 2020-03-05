// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository;

namespace AppInstaller::Workflow
{
    void ShowFlow::Execute()
    {
        WorkflowBase::IndexSearch();

        if (WorkflowBase::EnsureOneMatchFromSearchResult())
        {
            if (m_argsRef.Contains(ExecutionArgs::ExecutionArgType::ListVersions))
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
            m_argsRef.Contains(ExecutionArgs::ExecutionArgType::Version) ? *m_argsRef.GetArg(ExecutionArgs::ExecutionArgType::Version) : "",
            m_argsRef.Contains(ExecutionArgs::ExecutionArgType::Channel) ? *m_argsRef.GetArg(ExecutionArgs::ExecutionArgType::Channel) : ""
        );

        ManifestComparator manifestComparator(manifest, m_reporterRef);
        auto selectedLocalization = manifestComparator.GetPreferredLocalization(m_argsRef);
        auto selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);

        m_reporterRef.ShowMsg("Id: " + manifest.Id);
        m_reporterRef.ShowMsg("Name: " + manifest.Name);
        m_reporterRef.ShowMsg("Version: " + manifest.Version);
        m_reporterRef.ShowMsg("Author: " + manifest.Author);
        m_reporterRef.ShowMsg("AppMoniker: " + manifest.AppMoniker);
        m_reporterRef.ShowMsg("Description: " + selectedLocalization.Description);
        m_reporterRef.ShowMsg("Homepage: " + selectedLocalization.Homepage);
        m_reporterRef.ShowMsg("License: " + selectedLocalization.LicenseUrl);

        m_reporterRef.ShowMsg("Installer info:" + manifest.Id);
        m_reporterRef.ShowMsg("--Installer Language: " + selectedInstaller.Language);
        m_reporterRef.ShowMsg("--Installer SHA256: " + Utility::SHA256::ConvertToString(selectedInstaller.Sha256));
        m_reporterRef.ShowMsg("--Installer Download Url: " + selectedInstaller.Url);
        m_reporterRef.ShowMsg("--Installer Type: " + Manifest::ManifestInstaller::InstallerTypeToString(selectedInstaller.InstallerType));
    }

    void ShowFlow::ShowAppVersion()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        m_reporterRef.ShowMsg("Id: " + app->GetId());
        m_reporterRef.ShowMsg("Name: " + app->GetName());

        for (auto& version : app->GetVersions())
        {
            m_reporterRef.ShowMsg("  Version: " + version.first + ", Channel: " + version.second);
        }
    }
}