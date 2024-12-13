// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/FontFlow.h"
#include "Workflows/InstallFlow.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;
    using namespace std::string_view_literals;

    Utility::LocIndView s_FontCommand_HelpLink = "https://aka.ms/winget-command-font"_liv;

    std::vector<std::unique_ptr<Command>> FontCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<FontListCommand>(FullName()),
            std::make_unique<FontInstallCommand>(FullName()),
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

    std::vector<Argument> FontInstallCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Manifest),
            Argument{ Args::Type::InstallScope, Resource::String::InstallScopeDescription, ArgumentType::Standard, Argument::Visibility::Help },
        };
    }

    Resource::LocString FontInstallCommand::ShortDescription() const
    {
        return { Resource::String::FontInstallCommandShortDescription };
    }

    Resource::LocString FontInstallCommand::LongDescription() const
    {
        return { Resource::String::FontInstallCommandLongDescription };
    }

    void FontInstallCommand::Complete(Execution::Context& context, Args::Type valueType) const
    {
        UNREFERENCED_PARAMETER(valueType);
        context.Reporter.Error() << Resource::String::PendingWorkError << std::endl;
    }

    Utility::LocIndView FontInstallCommand::HelpLink() const
    {
        return s_FontCommand_HelpLink;
    }

    void FontInstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void FontInstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                Workflow::GetManifestFromArg <<
                Workflow::SelectInstaller <<
                Workflow::InstallSinglePackage;
        }
    }

    std::vector<Argument> FontListCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Family),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Source),
            Argument::ForType(Args::Type::Tag),
            Argument::ForType(Args::Type::Exact),
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
        UNREFERENCED_PARAMETER(valueType);
        context.Reporter.Error() << Resource::String::PendingWorkError << std::endl;
        THROW_HR(E_NOTIMPL);
    }

    Utility::LocIndView FontListCommand::HelpLink() const
    {
        return s_FontCommand_HelpLink;
    }

    void FontListCommand::ExecuteInternal(Execution::Context& context) const
    {
        context << Workflow::ReportInstalledFonts;
    }
}
