// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // IMPORTANT: This acts as a base interface for all COM commands, and should not be used directly
    // This will only all the COM Command implementations to share and enforce common behavior before invoking based implementation.
    // Right now, it sets the WinGetCOMApiCall flag to the context before invoking the base Execute implementation.
    struct COMCommand : public Command
    {
    protected:
        COMCommand(std::string_view name, std::string_view parent) : Command(name, parent) {}

    public:
        void Execute(Execution::Context& context) const override;
    };

    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    struct COMDownloadCommand final : public COMCommand
    {
        constexpr static std::string_view CommandName = "download"sv;
        COMDownloadCommand(std::string_view parent) : COMCommand(CommandName, parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    struct COMInstallCommand final : public COMCommand
    {
        constexpr static std::string_view CommandName = "install"sv;
        COMInstallCommand(std::string_view parent) : COMCommand(CommandName, parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    // IMPORTANT: To use this command, the caller should have already retrieved the InstalledPackageVersion and added it to the Context Data
    struct COMUninstallCommand final : public COMCommand
    {
        constexpr static std::string_view CommandName = "uninstall"sv;
        COMUninstallCommand(std::string_view parent) : COMCommand(CommandName, parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
