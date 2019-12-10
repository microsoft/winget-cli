// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Command.h"

namespace AppInstaller::CLI
{
    struct RootCommand final : public Command
    {
        RootCommand() : Command(L"root") {}

        virtual std::vector<std::unique_ptr<Command>> GetCommands() const override;

        virtual std::vector<std::wstring> GetLongDescription() const override;

    protected:
        virtual void ExecuteInternal(Invocation& inv, std::wostream& out) const;
    };
}
