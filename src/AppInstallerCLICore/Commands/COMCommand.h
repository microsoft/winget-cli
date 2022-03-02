// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    struct COMDownloadCommand final : public Command
    {
        constexpr static std::string_view CommandName = "download"sv;
        COMDownloadCommand(std::string_view parent) : Command(CommandName, parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    struct COMInstallCommand final : public Command
    {
        constexpr static std::string_view CommandName = "install"sv;
        COMInstallCommand(std::string_view parent) : Command(CommandName, parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // IMPORTANT: To use this command, the caller should have already retrieved the InstalledPackageVersion and added it to the Context Data
    struct COMUninstallCommand final : public Command
    {
        constexpr static std::string_view CommandName = "uninstall"sv;
        COMUninstallCommand(std::string_view parent) : Command(CommandName, parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
