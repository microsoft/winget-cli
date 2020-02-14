// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "InstallerHandlerBase.h"

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
        std::future<DWORD> ExecuteInstallerAsync(const std::filesystem::path& filePath, const std::string& args);

        // The known default arg format if the corresponding arg is not specified in the manifest
        // i.e. If silent switch is not specified in manifest and installer type is msi, /quiet will be returned.
        std::string GetDefaultArg(std::string_view argType);

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