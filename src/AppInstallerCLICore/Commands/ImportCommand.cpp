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
            Argument::ForType(Execution::Args::Type::Exact),
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

    void ImportCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::VerifyFile(Execution::Args::Type::ImportFile) <<
            Workflow::ReadImportFile <<
            Workflow::SearchPackagesForImport <<
            Workflow::InstallMultiple;
    }
}
