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
#include "ExportCommand.h"
#include "ImportCommand.h"

#include "Resources.h"
#include "TableOutput.h"

using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace
    {
        void OutputGroupPolicySourceList(Execution::Context& context, const std::vector<Settings::SourceFromPolicy>& sources, Resource::StringId header)
        {
            Execution::TableOutput<3> sourcesTable{ context.Reporter, { header, Resource::String::SourceListType, Resource::String::SourceListArg } };
            for (const auto& source : sources)
            {
                sourcesTable.OutputLine({ source.Name, source.Type, source.Arg });
            }

            sourcesTable.Complete();
        }

        void OutputGroupPolicies(Execution::Context& context)
        {
            const auto& groupPolicies = Settings::GroupPolicies();

            // Get the state of policies that are a simple enabled/disabled toggle
            std::map<Settings::TogglePolicy::Policy, Settings::PolicyState> activePolicies;
            for (const auto& togglePolicy : Settings::TogglePolicy::GetAllPolicies())
            {
                auto state = groupPolicies.GetState(togglePolicy.GetPolicy());
                if (state != Settings::PolicyState::NotConfigured)
                {
                    activePolicies[togglePolicy.GetPolicy()] = state;
                }
            }

            // The source update interval is the only ValuePolicy that is not gated by a TogglePolicy.
            // We need to output the table if there is a TogglePolicy configured or if this one is configured.
            // We can rework this when more policies are added.
            auto sourceAutoUpdateIntervalPolicy = groupPolicies.GetValue<Settings::ValuePolicy::SourceAutoUpdateIntervalInMinutes>();

            if (!activePolicies.empty() || sourceAutoUpdateIntervalPolicy.has_value())
            {
                auto info = context.Reporter.Info();
                info << std::endl;

                Execution::TableOutput<2> policiesTable{ context.Reporter, { Resource::String::PoliciesPolicy, Resource::String::PoliciesState } };

                // Output the toggle policies.
                for (const auto& activePolicy : activePolicies)
                {
                    auto policy = Settings::TogglePolicy::GetPolicy(activePolicy.first);
                    policiesTable.OutputLine({
                        Resource::LocString{ policy.PolicyName() }.get(),
                        Resource::LocString{ activePolicy.second == Settings::PolicyState::Enabled ? Resource::String::PoliciesEnabled : Resource::String::PoliciesDisabled }.get() });
                }

                // Output the update interval in the same table if needed.
                if (sourceAutoUpdateIntervalPolicy.has_value())
                {
                    policiesTable.OutputLine({
                        Resource::LocString{ AppInstaller::StringResource::String::PolicySourceAutoUpdateInterval },
                        std::to_string(sourceAutoUpdateIntervalPolicy.value()) });
                }

                policiesTable.Complete();

                // Output the additional and allowed sources as separate tables.
                if (groupPolicies.GetState(Settings::TogglePolicy::Policy::AdditionalSources) == Settings::PolicyState::Enabled)
                {
                    info << std::endl;
                    auto sources = groupPolicies.GetValueRef<Settings::ValuePolicy::AdditionalSources>();
                    if (sources.has_value() && !sources->get().empty())
                    {
                        OutputGroupPolicySourceList(context, sources->get(), Resource::String::SourceListAdditionalSource);
                    }
                }

                if (groupPolicies.GetState(Settings::TogglePolicy::Policy::AllowedSources) == Settings::PolicyState::Enabled)
                {
                    info << std::endl;
                    auto sources = groupPolicies.GetValueRef<Settings::ValuePolicy::AllowedSources>();
                    if (sources.has_value() && !sources->get().empty())
                    {
                        OutputGroupPolicySourceList(context, sources->get(), Resource::String::SourceListAllowedSource);
                    }
                }
            }
        }
    }

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
            std::make_unique<ExportCommand>(FullName()),
            std::make_unique<ImportCommand>(FullName()),
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

    void RootCommand::Execute(Execution::Context& context) const
    {
        AICLI_LOG(CLI, Info, << "Executing command: " << Name());
        if (context.Args.Contains(Execution::Args::Type::Help))
        {
            OutputHelp(context.Reporter);
        }
        else
        {
            ExecuteInternal(context);
        }
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

            OutputGroupPolicies(context);
        }
        else if (context.Args.Contains(Execution::Args::Type::ListVersions))
        {
            context.Reporter.Info() << 'v' << Runtime::GetClientVersion();
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }
}
