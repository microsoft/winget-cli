// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MsiInstallFlow.h"
#include "winget/MsiExecArguments.h"

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        std::optional<UINT> InvokeMsiInstallProduct(const std::filesystem::path& installerPath, const Msi::MsiParsedArguments& msiArgs, IProgressCallback&)
        {
            if (msiArgs.LogFile)
            {
                THROW_IF_WIN32_ERROR(MsiEnableLogW(msiArgs.LogMode, msiArgs.LogFile->c_str(), msiArgs.LogAttributes));
            }
            else
            {
                // Disable logging
                THROW_IF_WIN32_ERROR(MsiEnableLogW(0, nullptr, 0));
            }

            // Returns old UI level. We don't need to reset it so we ignore it.
            MsiSetInternalUI(msiArgs.UILevel, nullptr);

            // TODO: Use progress callback
            return MsiInstallProductW(installerPath.c_str(), msiArgs.Properties.c_str());
        }
    }

    void DirectMSIInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();
        const auto& additionalSuccessCodes = context.Get<Execution::Data::Installer>()->InstallerSuccessCodes;

        Msi::MsiParsedArguments parsedArgs = Msi::ParseMSIArguments(context.Get<Execution::Data::InstallerArgs>());

        auto installResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokeMsiInstallProduct,
                installerPath,
                parsedArgs,
                std::placeholders::_1));

        if (!installResult)
        {
            context.Reporter.Warn() << "Installation abandoned" << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else if (installResult.value() != 0 && (std::find(additionalSuccessCodes.begin(), additionalSuccessCodes.end(), installResult.value()) == additionalSuccessCodes.end()))
        {
            const auto& manifest = context.Get<Execution::Data::Manifest>();
            Logging::Telemetry().LogInstallerFailure(manifest.Id, manifest.Version, manifest.Channel, "ShellExecute", installResult.value());

            context.Reporter.Error() << "Installer failed with exit code: " << installResult.value() << std::endl;
            // Show installer log path if exists
            if (context.Contains(Execution::Data::LogPath) && std::filesystem::exists(context.Get<Execution::Data::LogPath>()))
            {
                context.Reporter.Info() << "Installer log is available at: " << context.Get<Execution::Data::LogPath>().u8string() << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SHELLEXEC_INSTALL_FAILED);
        }
        else
        {
            context.Reporter.Info() << Resource::String::InstallFlowInstallSuccess << std::endl;
        }
    }
}
