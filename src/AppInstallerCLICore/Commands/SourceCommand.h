// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct SourceCommand final : public Command
    {
        SourceCommand() : Command("source") {}

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const;
    };

    struct SourceAddCommand final : public Command
    {
        SourceAddCommand() : Command("add") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const;
    };

    struct SourceListCommand final : public Command
    {
        SourceListCommand() : Command("list") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const;
    };

    struct SourceUpdateCommand final : public Command
    {
        SourceUpdateCommand() : Command("update") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const;
    };

    struct SourceRemoveCommand final : public Command
    {
        SourceRemoveCommand() : Command("remove") {}

        virtual std::vector<Argument> GetArguments() const override;

        virtual std::string ShortDescription() const override;
        virtual std::vector<std::string> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const;
    };
}
