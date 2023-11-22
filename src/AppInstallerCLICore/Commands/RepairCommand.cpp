// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RepairCommand.h"

namespace AppInstaller::CLI
{
    std::vector<Argument> AppInstaller::CLI::RepairCommand::GetArguments() const
    {
        return std::vector<Argument>();
    }

    Resource::LocString AppInstaller::CLI::RepairCommand::ShortDescription() const
    {
        return { Resource::String::RepairCommandShortDescription };
    }

    Resource::LocString AppInstaller::CLI::RepairCommand::LongDescription() const
    {
        return { Resource::String::RepairCommandLongDescription };
    }

    void AppInstaller::CLI::RepairCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        UNREFERENCED_PARAMETER(context);
        UNREFERENCED_PARAMETER(valueType);
        // TODO: implement
    }

    Utility::LocIndView AppInstaller::CLI::RepairCommand::HelpLink() const
    {
        // TODO: point to the right place
        return "https://aka.ms/winget-command-repair"_liv;
    }

    void AppInstaller::CLI::RepairCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void AppInstaller::CLI::RepairCommand::ExecuteInternal(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        // TODO: implement
    }
}
