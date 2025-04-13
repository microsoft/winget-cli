// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscCommand.h"

#ifndef AICLI_DISABLE_TEST_HOOKS
#include "DscTestFileResource.h"
#include "DscTestJsonResource.h"
#endif

namespace AppInstaller::CLI
{
    std::vector<std::unique_ptr<Command>> DscCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
#ifndef AICLI_DISABLE_TEST_HOOKS
            std::make_unique<DscTestFileResource>(FullName()),
            std::make_unique<DscTestJsonResource>(FullName()),
#endif
        });
    }

    Resource::LocString DscCommand::ShortDescription() const
    {
        return { Resource::String::DscCommandShortDescription };
    }

    Resource::LocString DscCommand::LongDescription() const
    {
        return { Resource::String::DscCommandLongDescription };
    }

    Utility::LocIndView DscCommand::HelpLink() const
    {
        return "https://aka.ms/winget-dsc-resources"_liv;
    }

    void DscCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }
}
