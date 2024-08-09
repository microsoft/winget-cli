// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct SettingsCommand final : public Command
    {
        SettingsCommand(std::string_view parent) : Command("settings", { "config" }, parent, Settings::TogglePolicy::Policy::Settings) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;
        std::vector<Argument> GetArguments() const override;

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct SettingsExportCommand final : public Command
    {
        SettingsExportCommand(std::string_view parent) : Command("export", parent, CommandOutputFlags::IgnoreSettingsWarnings) {}

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct SettingsSetCommand final : public Command
    {
        SettingsSetCommand(std::string_view parent) : Command("set", {}, parent, Settings::TogglePolicy::Policy::Settings) {}

        std::vector<Argument> GetArguments() const override;

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct SettingsResetCommand final : public Command
    {
        SettingsResetCommand(std::string_view parent) : Command("reset", {}, parent, Settings::TogglePolicy::Policy::Settings) {}

        std::vector<Argument> GetArguments() const override;

        virtual Resource::LocString ShortDescription() const override;
        virtual Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
