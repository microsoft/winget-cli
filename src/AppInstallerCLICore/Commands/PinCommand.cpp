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
    using namespace AppInstaller::Utility::literals;
    using namespace std::string_view_literals;

    Utility::LocIndString s_PinCommand_HelpLink = "https://aka.ms/winget-command-pin"_lis;

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

    Utility::LocIndString PinCommand::HelpLink() const
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
            Argument{ "version"_liv, 'v', Args::Type::GatedVersion, Resource::String::GatedVersionArgumentDescription, ArgumentType::Standard },
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Args::Type::Force),
            Argument{ "blocking"_liv, Argument::NoAlias, Args::Type::BlockingPin, Resource::String::PinAddBlockingArgumentDescription, ArgumentType::Flag },
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
        switch (valueType)
        {
        case Args::Type::Query:
        case Args::Type::Id:
        case Args::Type::Name:
        case Args::Type::Moniker:
        case Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        }
    }

    Utility::LocIndString PinAddCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinAddCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::GatedVersion) && execArgs.Contains(Execution::Args::Type::BlockingPin))
        {
            throw CommandException(Resource::String::BothGatedVersionAndBlockingFlagProvided);
        }

    }

    void PinAddCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
        Command::ExecuteInternal(context);
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
            Argument::ForType(Args::Type::AcceptSourceAgreements),
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
        switch (valueType)
        {
        case Args::Type::Query:
        case Args::Type::Id:
        case Args::Type::Name:
        case Args::Type::Moniker:
        case Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        }
    }

    Utility::LocIndString PinRemoveCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinRemoveCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
        Command::ExecuteInternal(context);
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
        switch (valueType)
        {
        case Args::Type::Query:
        case Args::Type::Id:
        case Args::Type::Name:
        case Args::Type::Moniker:
        case Args::Type::Source:
            context <<
                Workflow::CompleteWithSingleSemanticsForValue(valueType);
            break;
        }
    }

    Utility::LocIndString PinListCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinListCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
        Command::ExecuteInternal(context);
    }

    std::vector<Argument> PinResetCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Force),
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

    Utility::LocIndString PinResetCommand::HelpLink() const
    {
        return s_PinCommand_HelpLink;
    }

    void PinResetCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
        Command::ExecuteInternal(context);
    }
}
