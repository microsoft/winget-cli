// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ShellExecuteInstallerHandler.h"
#include "MsixInstallerHandler.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::CLI::Workflow
{
    void EnsureMinOSVersion(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();

        if (!manifest.MinOSVersion.empty() &&
            !Runtime::IsCurrentOSVersionGreaterThanOrEqual(Version(manifest.MinOSVersion)))
        {
            context.Reporter.Error() << "Cannot install application, as it requires a higher OS version: " << manifest.MinOSVersion << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION));
        }
    }

    void EnsureApplicableInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();

        if (!installer.has_value())
        {
            context.Reporter.Error() << "No installers are applicable to the current system" << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }
    }

    void GetInstallerHandler(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        std::unique_ptr<InstallerHandlerBase> installerHandler;

        switch (installer->InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
        case ManifestInstaller::InstallerTypeEnum::Burn:
        case ManifestInstaller::InstallerTypeEnum::Inno:
        case ManifestInstaller::InstallerTypeEnum::Msi:
        case ManifestInstaller::InstallerTypeEnum::Nullsoft:
        case ManifestInstaller::InstallerTypeEnum::Wix:
            installerHandler = std::make_unique<ShellExecuteInstallerHandler>(installer.value(), context);
            break;
        case ManifestInstaller::InstallerTypeEnum::Msix:
            installerHandler = std::make_unique<MsixInstallerHandler>(installer.value(), context);
            break;
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }

        context.Add<Execution::Data::InstallerHandler>(std::move(installerHandler));
    }

    void DownloadInstaller(Execution::Context& context)
    {
        const auto& handler = context.Get<Execution::Data::InstallerHandler>();

        handler->Download();
    }

    void ExecuteInstaller(Execution::Context& context)
    {
        const auto& handler = context.Get<Execution::Data::InstallerHandler>();

        handler->Install();
    }
}
