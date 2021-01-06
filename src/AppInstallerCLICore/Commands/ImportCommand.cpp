// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ImportCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/ImportExportFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ImportCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ImportFile),
        };
    }

    Resource::LocString ImportCommand::ShortDescription() const
    {
        return { Resource::String::ImportCommandShortDescription };
    }

    Resource::LocString ImportCommand::LongDescription() const
    {
        return { Resource::String::ImportCommandLongDescription };
    }

    std::string ImportCommand::HelpLink() const
    {
        // TODO: point to correct location
        return "https://aka.ms/winget-command-import";
    }

    void ImportCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (!std::filesystem::exists(execArgs.GetArg(Execution::Args::Type::ImportFile)))
        {
            // TODO
            throw CommandException(Resource::String::VerifyFileFailedNotExist, execArgs.GetArg(Execution::Args::Type::ImportFile));
        }
    }

    void ImportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::ReadImportFile <<
            Workflow::SearchPackagesForImport <<
            Workflow::InstallMultiple;
    }
}
