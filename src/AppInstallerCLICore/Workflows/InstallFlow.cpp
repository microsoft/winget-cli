// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallFlow.h"
#include "ShellExecuteInstallerHandler.h"
#include "MsixInstallerHandler.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void InstallFlow::Execute()
    {
        if (m_argsRef.Contains(Execution::Args::Type::Manifest))
        {
            m_manifest = Manifest::Manifest::CreateFromPath(m_argsRef.GetArg(Execution::Args::Type::Manifest));
            Logging::Telemetry().LogManifestFields(m_manifest.Id, m_manifest.Name, m_manifest.Version);
        }
        else
        {
            if (!IndexSearch() || !EnsureOneMatchFromSearchResult() || !GetManifest())
            {
                return;
            }
            m_reporterRef.ShowMsg("Found app: " + m_searchResult.Matches[0].Application->GetName());
        }

        SelectInstaller();
        InstallInternal();
    }

    void InstallFlow::InstallInternal()
    {
        auto installerHandler = GetInstallerHandler();

        installerHandler->Download();
        installerHandler->Install();
    }

    std::unique_ptr<InstallerHandlerBase> InstallFlow::GetInstallerHandler()
    {
        switch (m_selectedInstaller.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            return std::make_unique<ShellExecuteInstallerHandler>(m_selectedInstaller, m_contextRef);
        case ManifestInstaller::InstallerTypeEnum::Msix:
            return std::make_unique<MsixInstallerHandler>(m_selectedInstaller, m_contextRef);
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }
}