// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RootCommand.h"

#include "InstallCommand.h"
#include "ShowCommand.h"
#include "SourceCommand.h"
#include "SearchCommand.h"
#include "ListCommand.h"
#include "UpgradeCommand.h"
#include "UninstallCommand.h"
#include "HashCommand.h"
#include "ValidateCommand.h"
#include "SettingsCommand.h"
#include "FeaturesCommand.h"
#include "ExperimentalCommand.h"
#include "CompleteCommand.h"

#include "Resources.h"
#include "TableOutput.h"

namespace AppInstaller::CLI
{
    using namespace Utility::literals;

    std::vector<std::unique_ptr<Command>> RootCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<InstallCommand>(FullName()),
            std::make_unique<ShowCommand>(FullName()),
            std::make_unique<SourceCommand>(FullName()),
            std::make_unique<SearchCommand>(FullName()),
            std::make_unique<ListCommand>(FullName()),
            std::make_unique<UpgradeCommand>(FullName()),
            std::make_unique<UninstallCommand>(FullName()),
            std::make_unique<HashCommand>(FullName()),
            std::make_unique<ValidateCommand>(FullName()),
            std::make_unique<SettingsCommand>(FullName()),
            std::make_unique<FeaturesCommand>(FullName()),
            std::make_unique<ExperimentalCommand>(FullName()),
            std::make_unique<CompleteCommand>(FullName()),
        });
    }

    std::vector<Argument> RootCommand::GetArguments() const
    {
        return
        {
            Argument{ "version", 'v', Execution::Args::Type::ListVersions, Resource::String::ToolVersionArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help },
            Argument{ "info", Argument::NoAlias, Execution::Args::Type::Info, Resource::String::ToolInfoArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help },
        };
    }

    Resource::LocString RootCommand::LongDescription() const
    {
        return { Resource::String::ToolDescription };
    }

    std::string RootCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-help";
    }

    void RootCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Info))
        {
            OutputIntroHeader(context.Reporter);

            auto info = context.Reporter.Info();

            info << std::endl <<
                "Windows: "_liv << Runtime::GetOSVersion() << std::endl;

            if (Runtime::IsRunningInPackagedContext())
            {
                info << Resource::String::Package << ": "_liv << Runtime::GetPackageVersion() << std::endl;
            };

            info << std::endl << Resource::String::Logs << ": "_liv << Runtime::GetPathTo(Runtime::PathName::DefaultLogLocationForDisplay).u8string() << std::endl;

            info << std::endl;

            Execution::TableOutput<2> links{ context.Reporter, { Resource::String::Links, {} } };

            links.OutputLine({ Resource::LocString(Resource::String::PrivacyStatement).get(), "https://aka.ms/winget-privacy" });
            links.OutputLine({ Resource::LocString(Resource::String::LicenseAgreement).get(), "https://aka.ms/winget-license" });
            links.OutputLine({ Resource::LocString(Resource::String::ThirdPartSoftwareNotices).get(), "https://aka.ms/winget-3rdPartyNotice" });
            links.OutputLine({ Resource::LocString(Resource::String::MainHomepage).get(), "https://aka.ms/winget" });

            links.Complete();
        }
        else if (context.Args.Contains(Execution::Args::Type::ListVersions))
        {
            context.Reporter.Info() << 'v' << Runtime::GetClientVersion() << ' ' << Resource::String::PreviewVersion;
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }
}
