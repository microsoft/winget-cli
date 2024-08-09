// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <winget/ExperimentalFeature.h>

namespace AppInstaller::CLI
{
    struct ConfigureCommand final : public Command
    {
        ConfigureCommand(std::string_view parent);

        std::vector<std::unique_ptr<Command>> GetCommands() const override;
        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void Complete(Execution::Context& context, Execution::Args::Type argType) const override;
    };
}
