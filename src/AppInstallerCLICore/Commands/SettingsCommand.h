// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct SettingsCommand final : public Command
    {
        SettingsCommand(std::string_view parent) : Command("settings", { "config" }, parent, Settings::TogglePolicy::Policy::Settings) {}

        std::vector<Argument> GetArguments() const override;

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

        std::string HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
