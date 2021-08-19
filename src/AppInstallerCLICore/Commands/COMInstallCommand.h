// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    // IMPORTANT: To use this command, the caller should have already retrieved the package manifest (GetManifest()) and added it to the Context Data
    struct COMInstallCommand final : public Command
    {
        COMInstallCommand(std::string_view parent) : Command("install", parent) {}

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
