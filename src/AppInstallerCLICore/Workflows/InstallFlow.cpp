// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallFlow.h"
#include "ManifestComparator.h"
#include "ShellExecuteInstallerHandler.h"
#include "MsixInstallerHandler.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void InstallFlow::Execute()
    {
        if (m_argsRef.Contains(ExecutionArgs::Type::Manifest))
        {
            m_manifest = Manifest::Manifest::CreateFromPath(*(m_argsRef.GetArg(ExecutionArgs::Type::Manifest)));
            InstallInternal();
        }
        else
        {
            WorkflowBase::IndexSearch();

            if (WorkflowBase::EnsureOneMatchFromSearchResult())
            {
                GetManifest();
                InstallInternal();
            }
        }
    }

    void InstallFlow::InstallInternal()
    {
        Logging::Telemetry().LogManifestFields(m_manifest.Name, m_manifest.Version);

        // Select Installer
        ManifestComparator manifestComparator(m_manifest, m_reporterRef);
        m_selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);

        auto installerHandler = GetInstallerHandler();

        installerHandler->Download();
        installerHandler->Install();
    }

    void InstallFlow::GetManifest()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        AICLI_LOG(CLI, Info, << "Found one app. App id: " << app->GetId() << " App name: " << app->GetName());
        m_reporterRef.ShowMsg("Found app: " + app->GetName());

        // Todo: handle failure if necessary after real search is in place
        m_manifest = app->GetManifest(
            m_argsRef.Contains(ExecutionArgs::Type::Version) ? *m_argsRef.GetArg(ExecutionArgs::Type::Version) : "",
            m_argsRef.Contains(ExecutionArgs::Type::Channel) ? *m_argsRef.GetArg(ExecutionArgs::Type::Channel) : ""
        );
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