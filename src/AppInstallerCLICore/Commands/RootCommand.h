// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct RootCommand final : public Command
    {
        constexpr static std::string_view CommandName = "root"sv;

        RootCommand() : Command(CommandName, {}) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;
        std::vector<Argument> GetArguments() const override;

        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

        void Execute(Execution::Context& context) const override;

    protected:
        virtual void ExecuteInternal(Execution::Context& context) const;
    };
}
