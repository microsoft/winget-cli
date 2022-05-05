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

        const auto& installer = context.Get<Execution::Data::Installer>();
        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();

        Msi::MsiParsedArguments parsedArgs = Msi::ParseMSIArguments(context.Get<Execution::Data::InstallerArgs>());

        // Inform of elevation requirements
        if (!Runtime::IsRunningAsAdmin() && installer->ElevationRequirement == Manifest::ElevationRequirementEnum::ElevatesSelf)
        {
            context.Reporter.Warn() << Resource::String::InstallerElevationExpected << std::endl;
        }

        auto installResult = context.Reporter.ExecuteWithProgress(
            std::bind(InvokeMsiInstallProduct,
                installerPath,
                parsedArgs,
                std::placeholders::_1));

        if (!installResult)
        {
            context.Reporter.Warn() << Resource::String::InstallationAbandoned << std::endl;
            AICLI_TERMINATE_CONTEXT(E_ABORT);
        }
        else
        {
            context.Add<Execution::Data::OperationReturnCode>(installResult.value());
        }
    }
}
