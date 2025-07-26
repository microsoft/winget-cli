// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/FontFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UninstallFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;
    using namespace std::string_view_literals;

    Utility::LocIndView s_FontCommand_HelpLink = "https://aka.ms/winget-command-font"_liv;

    // Base Font Command

    std::vector<std::unique_ptr<Command>> FontCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<FontListCommand>(FullName()),
            std::make_unique<FontInstallCommand>(FullName()),
            std::make_unique<FontUninstallCommand>(FullName()),
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

    // FontListCommand

    std::vector<Argument> FontListCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Family),
            Argument::ForType(Args::Type::Files),
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


    // FontInstallCommand

    std::vector<Argument> FontInstallCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Manifest),
            Argument{ Args::Type::InstallScope, Resource::String::InstallScopeDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument::ForType(Args::Type::Force),
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
        THROW_HR(E_NOTIMPL);
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

    // Font Uninstall Command

    std::vector<Argument> FontUninstallCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::Manifest),
            Argument{ Args::Type::InstallScope, Resource::String::InstalledScopeArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
        };
    }

    Resource::LocString FontUninstallCommand::ShortDescription() const
    {
        return { Resource::String::FontUninstallCommandShortDescription };
    }

    Resource::LocString FontUninstallCommand::LongDescription() const
    {
        return { Resource::String::FontUninstallCommandLongDescription };
    }

    void FontUninstallCommand::Complete(Execution::Context& context, Args::Type valueType) const
    {
        UNREFERENCED_PARAMETER(valueType);
        context.Reporter.Error() << Resource::String::PendingWorkError << std::endl;
        THROW_HR(E_NOTIMPL);
    }

    Utility::LocIndView FontUninstallCommand::HelpLink() const
    {
        return s_FontCommand_HelpLink;
    }

    void FontUninstallCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        Argument::ValidateCommonArguments(execArgs);
    }

    void FontUninstallCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);

        context <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(context));

        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                Workflow::GetManifestFromArg <<
                Workflow::FontUninstallImpl <<
                ReportUninstallerResult("FontUninstall"sv, APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED, true);
        }
    }
}
