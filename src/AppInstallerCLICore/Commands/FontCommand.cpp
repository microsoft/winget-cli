// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/FontFlow.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;
    using namespace std::string_view_literals;

    Utility::LocIndView s_FontCommand_HelpLink = "https://aka.ms/winget-command-Font"_liv;

    std::vector<std::unique_ptr<Command>> FontCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<FontListCommand>(FullName()),
        });
    }

    Resource::LocString FontCommand::ShortDescription() const
    {
        return { Resource::String::FontCommandShortDescription };
    }

    Resource::LocString FontCommand::LongDescription() const
    {
        return { Resource::String::FontCommandLongDescription };
    }

    Utility::LocIndView FontCommand::HelpLink() const
    {
        return s_FontCommand_HelpLink;
    }

    void FontCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    std::vector<Argument> FontListCommand::GetArguments() const
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

    Resource::LocString FontListCommand::ShortDescription() const
    {
        return { Resource::String::FontListCommandShortDescription };
    }

    Resource::LocString FontListCommand::LongDescription() const
    {
        return { Resource::String::FontListCommandLongDescription };
    }

    void FontListCommand::Complete(Execution::Context& context, Args::Type valueType) const
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

    Utility::LocIndView FontListCommand::HelpLink() const
    {
        return s_FontCommand_HelpLink;
    }

    void FontListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context << Workflow::ReportInstalledFontFamiliesResult;
    }
}
