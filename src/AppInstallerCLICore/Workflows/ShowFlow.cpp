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
        if (IndexSearch() && EnsureOneMatchFromSearchResult())
        {
            if (m_argsRef.Contains(Execution::Args::Type::ListVersions))
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
        if (GetManifest())
        {
            SelectInstaller();
            ManifestComparator manifestComparator(m_manifest, m_reporterRef);
            auto selectedLocalization = manifestComparator.GetPreferredLocalization(m_argsRef);

            m_reporterRef.ShowMsg("Id: " + m_manifest.Id);
            m_reporterRef.ShowMsg("Name: " + m_manifest.Name);
            m_reporterRef.ShowMsg("Version: " + m_manifest.Version);
            m_reporterRef.ShowMsg("Author: " + m_manifest.Author);
            m_reporterRef.ShowMsg("AppMoniker: " + m_manifest.AppMoniker);
            m_reporterRef.ShowMsg("Description: " + selectedLocalization.Description);
            m_reporterRef.ShowMsg("Homepage: " + selectedLocalization.Homepage);
            m_reporterRef.ShowMsg("License: " + selectedLocalization.LicenseUrl);

            m_reporterRef.ShowMsg("Installer info:" + m_manifest.Id);
            m_reporterRef.ShowMsg("--Installer Language: " + m_selectedInstaller.Language);
            m_reporterRef.ShowMsg("--Installer SHA256: " + Utility::SHA256::ConvertToString(m_selectedInstaller.Sha256));
            m_reporterRef.ShowMsg("--Installer Download Url: " + m_selectedInstaller.Url);
            m_reporterRef.ShowMsg("--Installer Type: " + Manifest::ManifestInstaller::InstallerTypeToString(m_selectedInstaller.InstallerType));
        }
    }

    void ShowFlow::ShowAppVersion()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        m_reporterRef.ShowMsg("Id: " + app->GetId());
        m_reporterRef.ShowMsg("Name: " + app->GetName());
        m_reporterRef.ShowMsg("Versions:");

        for (auto& version : app->GetVersions())
        {
            m_reporterRef.ShowMsg("  " + version.ToString());
        }
    }
}