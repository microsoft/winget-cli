// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PinCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/PinFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;
    using namespace std::string_view_literals;

    Utility::LocIndView s_PinCommand_HelpLink = "https://aka.ms/winget-command-pin"_liv;

    std::vector<std::unique_ptr<Command>> PinCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<PinAddCommand>(FullName()),
            std::make_unique<PinRemoveCommand>(FullName()),
            std::make_unique<PinListCommand>(FullName()),
            std::make_unique<PinResetCommand>(FullName()),
        });
    }

    Resource::LocString PinCommand::ShortDescription() const
    {
        return { Resource::String::PinCommandShortDescription };
    }

    Resource::LocString PinCommand::LongDescription() const
    {
        return { Resource::String::PinCommandLongDescription };
    }

    Utility::LocIndView PinCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    std::vector<Argument> PinAddCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Query),
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Tag),
            Argument::ForType(Args::Type::Command),
            Argument::ForType(Args::Type::Exact),
            Argument{ Args::Type::GatedVersion, Resource::String::GatedVersionArgumentDescription, ArgumentType::Standard },
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AuthenticationMode),
            Argument::ForType(Args::Type::AuthenticationAccount),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Args::Type::Force),
            Argument{ Args::Type::BlockingPin, Resource::String::PinAddBlockingArgumentDescription, ArgumentType::Flag },
            Argument{ Args::Type::PinInstalled, Resource::String::PinInstalledArgumentDescription, ArgumentType::Flag },
        };
    }

    Resource::LocString PinAddCommand::ShortDescription() const
    {
        return { Resource::String::PinAddCommandShortDescription };
    }

    Resource::LocString PinAddCommand::LongDescription() const
    {
        return { Resource::String::PinAddCommandLongDescription };
    }

    void PinAddCommand::Complete(Execution::Context& context, Args::Type valueType) const
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

    Utility::LocIndView PinAddCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinAddCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void PinAddCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Id))
        {
            // When we are given an ID, just pin that available package without checking for installed.
            // This helps when there are matching issues, for example due to multiple side-by-side installs.
            context <<
                Workflow::OpenSource();
        }
        else
        {
            // If not working from just ID, try matching a single installed package
            context <<
                Workflow::OpenSource() <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);
        }

        context <<
            Workflow::SearchSourceForSingle <<
            Workflow::HandleSearchResultFailures <<
            Workflow::EnsureOneMatchFromSearchResult(OperationType::Pin) <<
            Workflow::GetInstalledPackageVersion <<
            Workflow::ReportPackageIdentity <<
            Workflow::OpenPinningIndex() <<
            Workflow::AddPin;
    }

    std::vector<Argument> PinRemoveCommand::GetArguments() const
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
            Argument{ Args::Type::PinInstalled, Resource::String::PinInstalledArgumentDescription, ArgumentType::Flag },
        };
    }

    Resource::LocString PinRemoveCommand::ShortDescription() const
    {
        return { Resource::String::PinRemoveCommandShortDescription };
    }

    Resource::LocString PinRemoveCommand::LongDescription() const
    {
        return { Resource::String::PinRemoveCommandLongDescription };
    }

    void PinRemoveCommand::Complete(Execution::Context& context, Args::Type valueType) const
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

    Utility::LocIndView PinRemoveCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinRemoveCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Id))
        {
            // When we are given an ID, just un-pin that available package without checking for installed.
            // This helps when there are matching issues, for example due to multiple side-by-side installs.
            context <<
                Workflow::OpenSource();
        }
        else
        {
            // If not working from just ID, try matching a single installed package
            context <<
                Workflow::OpenSource() <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed);
        }

        context <<
            Workflow::SearchSourceForSingle <<
            Workflow::HandleSearchResultFailures <<
            Workflow::EnsureOneMatchFromSearchResult(OperationType::Pin) <<
            Workflow::GetInstalledPackageVersion <<
            Workflow::ReportPackageIdentity <<
            Workflow::OpenPinningIndex() <<
            Workflow::SearchPin <<
            Workflow::RemovePin;
    }

    std::vector<Argument> PinListCommand::GetArguments() const
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

    Resource::LocString PinListCommand::ShortDescription() const
    {
        return { Resource::String::PinListCommandShortDescription };
    }

    Resource::LocString PinListCommand::LongDescription() const
    {
        return { Resource::String::PinListCommandLongDescription };
    }

    void PinListCommand::Complete(Execution::Context& context, Args::Type valueType) const
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

    Utility::LocIndView PinListCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::OpenPinningIndex(/* readOnly */ true) <<
            Workflow::GetAllPins <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed, false, Repository::CompositeSearchBehavior::AllPackages) <<
            Workflow::ReportPins;
    }

    std::vector<Argument> PinResetCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Force),
            Argument::ForType(Args::Type::Source),
        };
    }

    Resource::LocString PinResetCommand::ShortDescription() const
    {
        return { Resource::String::PinResetCommandShortDescription };
    }

    Resource::LocString PinResetCommand::LongDescription() const
    {
        return { Resource::String::PinResetCommandLongDescription };
    }

    Utility::LocIndView PinResetCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinResetCommand::ExecuteInternal(Execution::Context& context) const
    {

        if (context.Args.Contains(Execution::Args::Type::Force))
        {
            context <<
                Workflow::OpenPinningIndex() <<
                Workflow::ResetAllPins;
        }
        else
        {
            AICLI_LOG(CLI, Info, << "--force argument is not present");
            context.Reporter.Info() << Resource::String::PinResetUseForceArg << std::endl;

            context <<
                Workflow::OpenPinningIndex(/* readOnly */ true) <<
                Workflow::GetAllPins <<
                Workflow::OpenSource() <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed) <<
                Workflow::ReportPins;
        }
    }
}
