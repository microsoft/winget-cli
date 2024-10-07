// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontsCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;
    using namespace std::string_view_literals;

    Utility::LocIndView s_FontsCommand_HelpLink = "https://aka.ms/winget-command-fonts"_liv;

    std::vector<std::unique_ptr<Command>> FontsCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<FontsListCommand>(FullName()),
        });
    }

    Resource::LocString FontsCommand::ShortDescription() const
    {
        return { Resource::String::FontsCommandShortDescription };
    }

    Resource::LocString FontsCommand::LongDescription() const
    {
        return { Resource::String::FontsCommandLongDescription };
    }

    Utility::LocIndView FontsCommand::HelpLink() const
    {
        return s_FontsCommand_HelpLink;
    }

    void FontsCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    std::vector<Argument> FontsListCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Query),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::Tag),
            Argument::ForType(Args::Type::Command),
            Argument::ForType(Args::Type::Exact),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AuthenticationMode),
            Argument::ForType(Args::Type::AuthenticationAccount),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
        };
    }

    Resource::LocString FontsListCommand::ShortDescription() const
    {
        return { Resource::String::FontsListCommandShortDescription };
    }

    Resource::LocString FontsListCommand::LongDescription() const
    {
        return { Resource::String::FontsListCommandLongDescription };
    }

    void FontsListCommand::Complete(Execution::Context& context, Args::Type valueType) const
    {
        context <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);

        switch (valueType)
        {
        case Execution::Args::Type::Query:
            context <<
                Workflow::RequireCompletionWordNonEmpty <<
                Workflow::SearchSourceForManyCompletion <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Source:
        case Execution::Args::Type::Tag:
        case Execution::Args::Type::Command:
            context <<
                Workflow::CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        }
    }

    Utility::LocIndView FontsListCommand::HelpLink() const
    {
        return s_FontsCommand_HelpLink;
    }

    void FontsListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);
    }
}
