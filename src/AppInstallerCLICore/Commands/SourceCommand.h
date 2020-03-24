// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct SourceCommand final : public Command
    {
        SourceCommand(std::string_view parent) : Command("source", parent) {}

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Execution::Context& context) const;
    };

    struct SourceAddCommand final : public Command
    {
        SourceAddCommand(std::string_view parent) : Command("add", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Execution::Context& context) const override;
    };

    struct SourceListCommand final : public Command
    {
        SourceListCommand(std::string_view parent) : Command("list", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Execution::Context& context) const override;
    };

    struct SourceUpdateCommand final : public Command
    {
        SourceUpdateCommand(std::string_view parent) : Command("update", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Execution::Context& context) const override;
    };

    struct SourceRemoveCommand final : public Command
    {
        SourceRemoveCommand(std::string_view parent) : Command("remove", parent) {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Execution::Context& context) const override;
    };
}
