// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Commands/Common.h"
#include "InstallFlow.h"
#include "ManifestComparator.h"
#include "ShellExecuteInstallerHandler.h"
#include "MsixInstallerHandler.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    void InstallFlow::Execute()
    {
        m_searchResult = WorkflowBase::IndexSearch();

        if (WorkflowBase::EnsureOneMatchFromSearchResult())
        {
            GetManifest();
            InstallInternal();
        }
    }

    void InstallFlow::Install(const Manifest::Manifest& manifest)
    {
        m_manifest = manifest;

        InstallInternal();
    }

    void InstallFlow::InstallInternal()
    {
        Logging::Telemetry().LogManifestFields(m_manifest.Name, m_manifest.Version);

        // Select Installer
        ManifestComparator manifestComparator(m_manifest, m_reporter);
        m_selectedInstaller = manifestComparator.GetPreferredInstaller(m_argsRef);

        auto installerHandler = GetInstallerHandler();

        installerHandler->Download();
        installerHandler->Install();
    }

    void InstallFlow::GetManifest()
    {
        auto app = m_searchResult.Matches.at(0).Application.get();

        AICLI_LOG(CLI, Info, << "Found one app. App id: " << app->GetId() << " App name: " << app->GetName());
        m_reporter.ShowMsg(WorkflowReporter::Level::Info, "Found app: " + app->GetName());

        // Todo: handle failure if necessary after real search is in place
        m_manifest = app->GetManifest(
            m_argsRef.Contains(CLI::ARG_VERSION) ? *m_argsRef.GetArg(CLI::ARG_VERSION) : "",
            m_argsRef.Contains(CLI::ARG_CHANNEL) ? *m_argsRef.GetArg(CLI::ARG_CHANNEL) : ""
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
            return std::make_unique<ShellExecuteInstallerHandler>(m_selectedInstaller, m_argsRef, m_reporter);
        case ManifestInstaller::InstallerTypeEnum::Msix:
            return std::make_unique<MsixInstallerHandler>(m_selectedInstaller, m_argsRef, m_reporter);
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }
}