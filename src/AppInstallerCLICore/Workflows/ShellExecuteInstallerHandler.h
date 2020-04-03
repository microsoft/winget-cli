// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include "ExecutionContext.h"

#include <filesystem>
#include <optional>

namespace AppInstaller::CLI::Workflow
{
    // ShellExecuteInstallerHandler handles installers run through ShellExecute.
    // Exe, Wix, Nullsoft, Msi and Inno should be handled by this installer handler.
    class ShellExecuteInstallerHandler
    {
    public:
        ShellExecuteInstallerHandler() = default;

        // Install is done though invoking ShellExecute on downloaded installer.
        void Install(Execution::Context& context);

    protected:
        static std::optional<DWORD> ExecuteInstaller(const std::filesystem::path& filePath, const std::string& args, IProgressCallback& progress);

        // Construct the installer arg string from appropriate source(known args, manifest) according to command line args.
        // Token is not replaced with actual values yet.
        std::string GetInstallerArgsTemplate(Execution::Context& context);

        // Replace tokens in the installer arg string with appropriate values.
        void PopulateInstallerArgsTemplate(Execution::Context& context, std::string& installerArgs);

        // Gets the installer args from the context.
        std::string GetInstallerArgs(Execution::Context& context);

        // This method appends appropriate extension to the downloaded installer.
        // ShellExecute uses file extension to launch the installer appropriately.
        virtual void RenameDownloadedInstaller(Execution::Context& context);

        std::filesystem::path m_logLocation;
    };
}