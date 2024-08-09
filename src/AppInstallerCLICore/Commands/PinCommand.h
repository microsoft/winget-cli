// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    struct PinCommand final : public Command
    {
        PinCommand(std::string_view parent) : Command("pin", {} /* aliases */, parent) {}

        std::vector<std::unique_ptr<Command>> GetCommands() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct PinAddCommand final : public Command
    {
        PinAddCommand(std::string_view parent) : Command("add", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ValidateArgumentsInternal(Execution::Args& execArgs) const override;
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct PinRemoveCommand final : public Command
    {
        PinRemoveCommand(std::string_view parent) : Command("remove", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct PinListCommand final : public Command
    {
        PinListCommand(std::string_view parent) : Command("list", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        void Complete(Execution::Context& context, Execution::Args::Type valueType) const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };

    struct PinResetCommand final : public Command
    {
        PinResetCommand(std::string_view parent) : Command("reset", parent) {}

        std::vector<Argument> GetArguments() const override;

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

        Utility::LocIndView HelpLink() const override;

    protected:
        void ExecuteInternal(Execution::Context& context) const override;
    };
}
