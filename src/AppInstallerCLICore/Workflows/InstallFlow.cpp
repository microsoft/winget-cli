// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "InstallFlow.h"
#include "ManifestComparator.h"
#include "ShellExecuteInstallerHandler.h"
#include "MsixInstallerHandler.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow {

    void InstallFlow::Install()
    {
        ProcessManifest();

        auto installerHandler = GetInstallerHandler();

        installerHandler->Download();
        installerHandler->Install();
    }

    void InstallFlow::ProcessManifest()
    {
        ManifestComparator manifestComparator(m_packageManifest, m_reporter);

        m_selectedLocalization = manifestComparator.GetPreferredLocalization(std::locale(""));

        m_reporter.ShowPackageInfo(
            m_packageManifest.Name,
            m_packageManifest.Version,
            m_packageManifest.Author,
            m_selectedLocalization.Description,
            m_selectedLocalization.Homepage,
            m_selectedLocalization.LicenseUrl
        );

        m_selectedInstaller = manifestComparator.GetPreferredInstaller(std::locale(""));
    }

    std::unique_ptr<InstallerHandlerBase> InstallFlow::GetInstallerHandler()
    {
        switch (m_selectedInstaller.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            return std::make_unique<ShellExecuteInstallerHandler>(m_selectedInstaller, m_reporter);
        case ManifestInstaller::InstallerTypeEnum::Msix:
            return std::make_unique<MsixInstallerHandler>(m_selectedInstaller, m_reporter);
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }
}