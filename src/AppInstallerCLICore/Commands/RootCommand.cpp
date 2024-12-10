// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RootCommand.h"
#include <AppInstallerRuntime.h>

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
#include "FontCommand.h"
#include "ExperimentalCommand.h"
#include "CompleteCommand.h"
#include "ExportCommand.h"
#include "ImportCommand.h"
#include "PinCommand.h"
#include "ConfigureCommand.h"
#include "DebugCommand.h"
#include "TestCommand.h"
#include "DownloadCommand.h"
#include "ErrorCommand.h"
#include "ResumeCommand.h"
#include "RepairCommand.h"

#include "Resources.h"
#include "TableOutput.h"

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Utility::literals;
    using namespace Settings;

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

                Execution::TableOutput<2> policiesTable{ context.Reporter, { Resource::String::PoliciesPolicy, Resource::String::StateHeader } };

                // Output the toggle policies.
                for (const auto& activePolicy : activePolicies)
                {
                    auto policy = Settings::TogglePolicy::GetPolicy(activePolicy.first);
                    policiesTable.OutputLine({
                        Resource::LocString{ policy.PolicyName() }.get(),
                        Resource::LocString{ activePolicy.second == Settings::PolicyState::Enabled ? Resource::String::StateEnabled : Resource::String::StateDisabled }.get() });
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
                info << std::endl;
            }
        }

        void OutputAdminSettings(Execution::Context& context)
        {
            Execution::TableOutput<2> adminSettingsTable{ context.Reporter, { Resource::String::AdminSettingHeader, Resource::String::StateHeader } };

            // Output the admin settings.
            for (const auto& setting : Settings::GetAllBoolAdminSettings())
            {
                adminSettingsTable.OutputLine({
                    std::string{ AdminSettingToString(setting)},
                    Resource::LocString{ IsAdminSettingEnabled(setting) ? Resource::String::StateEnabled : Resource::String::StateDisabled }
                });
            }
            for (const auto& setting : Settings::GetAllStringAdminSettings())
            {
                auto settingValue = GetAdminSetting(setting);
                adminSettingsTable.OutputLine({
                    std::string{ AdminSettingToString(setting)},
                    settingValue ? Utility::LocIndString{ settingValue.value() } : Resource::LocString{ Resource::String::StateDisabled }
                    });
            }
            adminSettingsTable.Complete();
        }

        void OutputKeyDirectories(Execution::Context& context)
        {
            Execution::TableOutput<2> keyDirectories{ context.Reporter, { Resource::String::KeyDirectoriesHeader, {} } };
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::Logs }, Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::UserSettings }, UserSettings::SettingsFilePath(true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::PortableLinksUser }, Runtime::GetPathTo(Runtime::PathName::PortableLinksUserLocation, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::PortableLinksMachine }, Runtime::GetPathTo(Runtime::PathName::PortableLinksMachineLocation, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::PortableRootUser }, Runtime::GetPathTo(Runtime::PathName::PortablePackageUserRoot, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::PortableRoot }, Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRoot, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::PortableRoot86 }, Runtime::GetPathTo(Runtime::PathName::PortablePackageMachineRootX86, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::InstallerDownloads }, Runtime::GetPathTo(Runtime::PathName::UserProfileDownloads, true).u8string() });
            keyDirectories.OutputLine({ Resource::LocString{ Resource::String::ConfigurationModules }, Runtime::GetPathTo(Runtime::PathName::ConfigurationModules, true).u8string() });
            keyDirectories.Complete();
            context.Reporter.Info() << std::endl;
        }

        void OutputLinks(Execution::Context& context)
        {
            Execution::TableOutput<2> links{ context.Reporter, { Resource::String::Links, {} } };
            links.OutputLine({ Resource::LocString{ Resource::String::PrivacyStatement }, "https://aka.ms/winget-privacy" });
            links.OutputLine({ Resource::LocString{ Resource::String::LicenseAgreement }, "https://aka.ms/winget-license" });
            links.OutputLine({ Resource::LocString{ Resource::String::ThirdPartSoftwareNotices }, "https://aka.ms/winget-3rdPartyNotice" });
            links.OutputLine({ Resource::LocString{ Resource::String::MainHomepage }, "https://aka.ms/winget" });
            links.OutputLine({ Resource::LocString{ Resource::String::WindowsStoreTerms }, "https://www.microsoft.com/en-us/storedocs/terms-of-sale" });
            links.Complete();
            context.Reporter.Info() << std::endl;
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
            std::make_unique<PinCommand>(FullName()),
            std::make_unique<ConfigureCommand>(FullName()),
            std::make_unique<DownloadCommand>(FullName()),
            std::make_unique<ErrorCommand>(FullName()),
            std::make_unique<ResumeCommand>(FullName()),
            std::make_unique<RepairCommand>(FullName()),
            std::make_unique<FontCommand>(FullName()),
#if _DEBUG
            std::make_unique<DebugCommand>(FullName()),
#endif
#ifndef AICLI_DISABLE_TEST_HOOKS
            std::make_unique<TestCommand>(FullName()),
#endif
        });
    }

    std::vector<Argument> RootCommand::GetArguments() const
    {
        return
        {
            Argument{ Execution::Args::Type::ToolVersion, Resource::String::ToolVersionArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help },
            Argument{ Execution::Args::Type::Info, Resource::String::ToolInfoArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help },
        };
    }

    Resource::LocString RootCommand::LongDescription() const
    {
        return { Resource::String::ToolDescription };
    }

    Utility::LocIndView RootCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-help"_liv;
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

        if (context.Args.Contains(Execution::Args::Type::OpenLogs))
        {
            ShellExecute(NULL, NULL, Runtime::GetPathTo(Runtime::PathName::DefaultLogLocation).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
        }

        if (context.Args.Contains(Execution::Args::Type::Wait))
        {
            context.Reporter.PromptForEnter();
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

            info << Resource::String::SystemArchitecture(Utility::ToString(Utility::GetSystemArchitecture())) << std::endl;

            if (Runtime::IsRunningInPackagedContext())
            {
                info << Resource::String::Package(Runtime::GetPackageVersion()) << std::endl;
            };

            info << std::endl;

            OutputKeyDirectories(context);
            OutputLinks(context);
            OutputGroupPolicies(context);
            OutputAdminSettings(context);
        }
        else if (context.Args.Contains(Execution::Args::Type::ToolVersion))
        {
            context.Reporter.Info() << 'v' << Runtime::GetClientVersion() << std::endl;
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }
}
