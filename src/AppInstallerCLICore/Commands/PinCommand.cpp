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
    using namespace std::string_view_literals;

    // TODO: Create this link with the docs!
    static constexpr std::string_view s_PinCommand_HelpLink = "https://aka.ms/winget-command-pin"sv;

    std::vector<std::unique_ptr<Command>> PinCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<PinAddCommand>(FullName()),
            std::make_unique<PinRemoveCommand>(FullName()),
            std::make_unique<PinListCommand>(FullName()),
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

    std::string PinCommand::HelpLink() const
    {
        return std::string{ s_PinCommand_HelpLink };
    }

    void PinCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    std::vector<Argument> PinAddCommand::GetArguments() const
    {
        // TODO
    }

    Resource::LocString PinAddCommand::ShortDescription() const
    {
        return { Resource::String::PinAddCommandShortDescription };
    }

    Resource::LocString PinAddCommand::LongDescription() const
    {
        return { Resource::String::PinAddCommandLongDescription };
    }

    void PinAddCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        // TODO
    }

    std::string PinAddCommand::HelpLink() const
    {
        return std::string{ s_PinCommand_HelpLink };
    }

    void PinAddCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
    }

    std::vector<Argument> PinRemoveCommand::GetArguments() const
    {
        // TODO
    }

    Resource::LocString PinRemoveCommand::ShortDescription() const
    {
        return { Resource::String::PinRemoveCommandShortDescription };
    }

    Resource::LocString PinRemoveCommand::LongDescription() const
    {
        return { Resource::String::PinRemoveCommandLongDescription };
    }

    void PinRemoveCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        // TODO
    }

    std::string PinRemoveCommand::HelpLink() const
    {
        return std::string{ s_PinCommand_HelpLink };
    }

    void PinRemoveCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
    }

    std::vector<Argument> PinListCommand::GetArguments() const
    {
        // TODO
    }

    Resource::LocString PinListCommand::ShortDescription() const
    {
        return { Resource::String::PinListCommandShortDescription };
    }

    Resource::LocString PinListCommand::LongDescription() const
    {
        return { Resource::String::PinListCommandLongDescription };
    }

    void PinListCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        // TODO
    }

    std::string PinListCommand::HelpLink() const
    {
        return std::string{ s_PinCommand_HelpLink };
    }

    void PinListCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
    }

    std::vector<Argument> PinResetCommand::GetArguments() const
    {
        // TODO
    }

    Resource::LocString PinResetCommand::ShortDescription() const
    {
        return { Resource::String::PinResetCommandShortDescription };
    }

    Resource::LocString PinResetCommand::LongDescription() const
    {
        return { Resource::String::PinResetCommandLongDescription };
    }

    void PinResetCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        // TODO
    }

    std::string PinResetCommand::HelpLink() const
    {
        return std::string{ s_PinCommand_HelpLink };
    }

    void PinResetCommand::ExecuteInternal(Execution::Context& context) const
    {
        // TODO
    }
}
