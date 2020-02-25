// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallerHandlerBase.h"
#include <AppInstallerProgress.h>

#include <optional>

namespace AppInstaller::Workflow
{
    // ShellExecuteInstallerHandler handles installers run through ShellExecute.
    // Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
    class ShellExecuteInstallerHandler : public InstallerHandlerBase
    {
    public:
        ShellExecuteInstallerHandler(
            const Manifest::ManifestInstaller& manifestInstaller,
            const CLI::Invocation& args,
            WorkflowReporter& reporter) :
            InstallerHandlerBase(manifestInstaller, args, reporter) {};

        // Install is done though invoking SheelExecute on downloaded installer.
        void Install() override;

    protected:
        static std::optional<DWORD> ExecuteInstaller(const std::filesystem::path& filePath, const std::string& args, bool interactive, IProgressCallback& progress);

        // Construct the installer arg string from appropriate source(known args, manifest) according to command line args.
        // Token is not replaced with actual values yet.
        std::string GetInstallerArgsTemplate();

        // Replace tokens in the installer arg string with appropriate values.
        void PopulateInstallerArgsTemplate(std::string& installerArgs);

        std::string GetInstallerArgs();

        // This method appends appropriate extension to the downloaded installer.
        // ShellExecute uses file extension to launch the installer appropriately.
        virtual void RenameDownloadedInstaller();
    };
}