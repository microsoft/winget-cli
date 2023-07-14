// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ResumeCommand.h"
#include "RootCommand.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace Execution;

    std::vector<Argument> ResumeCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ResumeGuid),
        };
    }

    Resource::LocString ResumeCommand::ShortDescription() const
    {
        return { Resource::String::ResumeCommandShortDescription };
    }

    Resource::LocString ResumeCommand::LongDescription() const
    {
        return { Resource::String::ResumeCommandLongDescription };
    }

    Utility::LocIndView ResumeCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-resume"_liv;
    }

    void ResumeCommand::ExecuteInternal(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
    }
}
