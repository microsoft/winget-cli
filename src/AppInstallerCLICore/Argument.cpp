// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Argument.h"
#include "Command.h"
#include "Resources.h"
#include <winget/UserSettings.h>
#include <winget/LocIndependent.h>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace Settings;
    using namespace AppInstaller::Utility::literals;

    namespace
    {
        bool ContainsArgumentFromList(const Execution::Args& args, const std::vector<Execution::Args::Type>& argTypes)
        {
            return std::any_of(argTypes.begin(), argTypes.end(), [&](Execution::Args::Type arg) { return args.Contains(arg); });
        }
    }

    ArgumentCommon ArgumentCommon::ForType(Execution::Args::Type type)
    {
        // A test ensures that all types are listed here
        switch (type)
        {
        // Args to specify where to get app
        case Execution::Args::Type::Query:
            return { type, "query"_liv, 'q', ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::MultiQuery:
            return { type, "query"_liv, 'q', ArgTypeCategory::PackageQuery | ArgTypeCategory::MultiplePackages };
        case Execution::Args::Type::Manifest:
            return { type, "manifest"_liv, 'm', ArgTypeCategory::Manifest };

        // Query filtering criteria and query behavior
        case Execution::Args::Type::Id:
            return { type, "id"_liv, ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::Name:
            return { type, "name"_liv, ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::Moniker:
            return { type, "moniker"_liv, ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::Tag:
            return { type, "tag"_liv, ArgTypeCategory::PackageQuery };
        case Execution::Args::Type::Command:
            return { type, "command"_liv, "cmd"_liv, ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::Source:
            return { type, "source"_liv, 's', ArgTypeCategory::QuerySource };
        case Execution::Args::Type::Count:
            return { type, "count"_liv, 'n', ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::Exact:
            return { type, "exact"_liv, 'e', ArgTypeCategory::PackageQuery };

        // Manifest selection behavior after an app is found
        case Execution::Args::Type::Version:
            return { type, "version"_liv, 'v', ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::Channel:
            return { type, "channel"_liv, 'c', ArgTypeCategory::PackageQuery };

        // Install behavior
        case Execution::Args::Type::Interactive:
            return { type, "interactive"_liv, 'i', ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::Silent:
            return { type, "silent"_liv, 'h', ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::Locale:
            return { type, "locale"_liv, ArgTypeCategory::InstallerSelection | ArgTypeCategory::CopyValueToSubContext };
        case Execution::Args::Type::Log:
            return { type, "log"_liv, 'o', ArgTypeCategory::InstallerSelection | ArgTypeCategory::SingleInstallerBehavior };
        case Execution::Args::Type::CustomSwitches:
            return { type, "custom"_liv, ArgTypeCategory::InstallerSelection | ArgTypeCategory::SingleInstallerBehavior };
        case Execution::Args::Type::Override:
            return { type, "override"_liv, ArgTypeCategory::InstallerSelection | ArgTypeCategory::SingleInstallerBehavior };
        case Execution::Args::Type::InstallLocation:
            return { type, "location"_liv, 'l', ArgTypeCategory::InstallerSelection | ArgTypeCategory::SingleInstallerBehavior };
        case Execution::Args::Type::InstallScope:
            return { type, "scope"_liv, ArgTypeCategory::InstallerSelection | ArgTypeCategory::CopyValueToSubContext };
        case Execution::Args::Type::InstallArchitecture:
            return { type, "architecture"_liv, 'a', ArgTypeCategory::InstallerSelection | ArgTypeCategory::CopyValueToSubContext };
        case Execution::Args::Type::InstallerArchitecture: // Used for input architecture that does not need applicability check. E.g. Download, Show.
            return { type, "architecture"_liv, 'a', ArgTypeCategory::InstallerSelection | ArgTypeCategory::CopyValueToSubContext };
        case Execution::Args::Type::InstallerType:
            return { type, "installer-type"_liv, ArgTypeCategory::InstallerSelection };
        case Execution::Args::Type::HashOverride:
            return { type, "ignore-security-hash"_liv, ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::IgnoreLocalArchiveMalwareScan:
            return { type, "ignore-local-archive-malware-scan"_liv, ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::AcceptPackageAgreements:
            return { type, "accept-package-agreements"_liv, ArgTypeCategory::InstallerBehavior };
        case Execution::Args::Type::Rename:
            return { type, "rename"_liv, 'r' };
        case Execution::Args::Type::NoUpgrade:
            return { type, "no-upgrade"_liv, ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::SkipDependencies:
            return { type, "skip-dependencies"_liv, ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::AllowReboot:
            return { type, "allow-reboot"_liv, ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };

        // Uninstall behavior
        case Execution::Args::Type::Purge:
            return { type, "purge"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::PurgePreserve };
        case Execution::Args::Type::Preserve:
            return { type, "preserve"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::PurgePreserve };
        case Execution::Args::Type::ProductCode:
            return { type, "product-code"_liv, ArgTypeCategory::SinglePackageQuery };
        case Execution::Args::Type::AllVersions:
            return { type, "all-versions"_liv, "all"_liv, ArgTypeCategory::CopyFlagToSubContext, ArgTypeExclusiveSet::AllAndTargetVersion };
        case Execution::Args::Type::TargetVersion:
            return { type, "version"_liv, 'v', ArgTypeCategory::SinglePackageQuery, ArgTypeExclusiveSet::AllAndTargetVersion };

        //Source Command
        case Execution::Args::Type::SourceName:
            return { type, "name"_liv, 'n' };
        case Execution::Args::Type::SourceType:
            return { type, "type"_liv, 't' };
        case Execution::Args::Type::SourceArg:
            return { type, "arg"_liv, 'a' };
        case Execution::Args::Type::ForceSourceReset:
            return { type, "force"_liv };
        case Execution::Args::Type::SourceExplicit:
            return { type, "explicit"_liv };
        case Execution::Args::Type::SourceTrustLevel:
            return { type, "trust-level"_liv };

        //Hash Command
        case Execution::Args::Type::HashFile:
            return { type, "file"_liv, 'f' };
        case Execution::Args::Type::Msix:
            return { type, "msix"_liv, 'm' };

        //Validate Command
        case Execution::Args::Type::ValidateManifest:
            return { type, "manifest"_liv };
        case Execution::Args::Type::IgnoreWarnings:
            return { type, "ignore-warnings"_liv, "nowarn"_liv};

        // Complete Command
        case Execution::Args::Type::Word:
            return { type, "word"_liv };
        case Execution::Args::Type::CommandLine:
            return { type, "commandline"_liv };
        case Execution::Args::Type::Position:
            return { type, "position"_liv };

        // Export Command
        case Execution::Args::Type::IncludeVersions:
            return { type, "include-versions"_liv };

        // Import Command
        case Execution::Args::Type::ImportFile:
            return { type, "import-file"_liv, 'i' };
        case Execution::Args::Type::IgnoreUnavailable:
            return { type, "ignore-unavailable"_liv };
        case Execution::Args::Type::IgnoreVersions:
            return { type, "ignore-versions"_liv };

        // Setting Command
        case Execution::Args::Type::AdminSettingEnable:
            return { type, "enable"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::EnableDisable };
        case Execution::Args::Type::AdminSettingDisable:
            return { type, "disable"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::EnableDisable };
        case Execution::Args::Type::SettingName:
            return { type, "setting"_liv };
        case Execution::Args::Type::SettingValue:
            return { type, "value"_liv };

        // Upgrade command
        case Execution::Args::Type::All:
            return { type, "all"_liv, 'r', "recurse"_liv, ArgTypeCategory::MultiplePackages };
        case Execution::Args::Type::IncludeUnknown:
            return { type, "include-unknown"_liv, 'u', "unknown"_liv, ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::IncludePinned:
            return { type, "include-pinned"_liv, "pinned"_liv, ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::UninstallPrevious:
            return { type, "uninstall-previous"_liv, ArgTypeCategory::InstallerBehavior | ArgTypeCategory::CopyFlagToSubContext };

        // Show command
        case Execution::Args::Type::ListVersions:
            return { type, "versions"_liv };

        // List command
        case Execution::Args::Type::Upgrade:
            return { type, "upgrade-available"_liv};

        // Pin command
        case Execution::Args::Type::GatedVersion:
            return { type, "version"_liv, 'v', ArgTypeCategory::None, ArgTypeExclusiveSet::PinType };
        case Execution::Args::Type::BlockingPin:
            return { type, "blocking"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::PinType };
        case Execution::Args::Type::PinInstalled:
            return { type, "installed"_liv, ArgTypeCategory::None };

        // Error command
        case Execution::Args::Type::ErrorInput:
            return { type, "input"_liv, ArgTypeCategory::None };

        // Resume command
        case Execution::Args::Type::ResumeId:
            return { type, "resume-id"_liv, 'g', ArgTypeCategory::None };
        case Execution::Args::Type::IgnoreResumeLimit:
            return { type, "ignore-resume-limit"_liv, ArgTypeCategory::None };

        // Font command
        case Execution::Args::Type::Family:
            return { type, "family"_liv, ArgTypeCategory::None };

        // Configuration commands
        case Execution::Args::Type::ConfigurationFile:
            return { type, "file"_liv, 'f', ArgTypeCategory::ConfigurationSetChoice, ArgTypeExclusiveSet::ConfigurationSetChoice };
        case Execution::Args::Type::ConfigurationAcceptWarning:
            return { type, "accept-configuration-agreements"_liv };
        case Execution::Args::Type::ConfigurationSuppressPrologue:
            return { type, "suppress-initial-details"_liv };
        case Execution::Args::Type::ConfigurationEnable:
            return { type, "enable"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::StubType };
        case Execution::Args::Type::ConfigurationDisable:
            return { type, "disable"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::StubType };
        case Execution::Args::Type::ConfigurationModulePath:
            return { type, "module-path"_liv };
        case Execution::Args::Type::ConfigurationExportPackageId:
            return { type, "package-id"_liv };
        case Execution::Args::Type::ConfigurationExportModule:
            return { type, "module"_liv };
        case Execution::Args::Type::ConfigurationExportResource:
            return { type, "resource"_liv };
        case Execution::Args::Type::ConfigurationExportAll:
            return { type, "all"_liv, 'r', "recurse"_liv };
        case Execution::Args::Type::ConfigurationHistoryItem:
            return { type, "history"_liv, 'h', ArgTypeCategory::ConfigurationSetChoice, ArgTypeExclusiveSet::ConfigurationSetChoice };
        case Execution::Args::Type::ConfigurationHistoryRemove:
            return { type, "remove"_liv };
        case Execution::Args::Type::ConfigurationStatusWatch:
            return { type, "live"_liv };

        // Download command
        case Execution::Args::Type::DownloadDirectory:
            return { type, "download-directory"_liv, 'd', ArgTypeCategory::None };
        case Execution::Args::Type::Platform:
            return { type, "platform"_liv, ArgTypeCategory::None };
        case Execution::Args::Type::SkipMicrosoftStorePackageLicense:
            return { type, "skip-microsoft-store-package-license"_liv, "skip-license"_liv, ArgTypeCategory::None };

        // Common arguments
        case Execution::Args::Type::NoVT:
            return { type, "no-vt"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::ProgressBarOption };
        case Execution::Args::Type::RetroStyle:
            return { type, "retro"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::ProgressBarOption };
        case Execution::Args::Type::RainbowStyle:
            return { type, "rainbow"_liv, ArgTypeCategory::None, ArgTypeExclusiveSet::ProgressBarOption };
        case Execution::Args::Type::Help:
            return { type, "help"_liv, APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR };
        case Execution::Args::Type::Info:
            return { type, "info"_liv };
        case Execution::Args::Type::VerboseLogs:
            return { type, "verbose-logs"_liv, "verbose"_liv };
        case Execution::Args::Type::DisableInteractivity:
            return { type, "disable-interactivity"_liv };
        case Execution::Args::Type::Wait:
            return { type, "wait"_liv };
        case Execution::Args::Type::OpenLogs:
            return { type, "open-logs"_liv, "logs"_liv };
        case Execution::Args::Type::Force:
            return { type, "force"_liv, ArgTypeCategory::CopyFlagToSubContext };
        case Execution::Args::Type::OutputFile:
            return { type, "output"_liv, 'o' };

        case Execution::Args::Type::DependencySource:
            return { type, "dependency-source"_liv, ArgTypeCategory::ExtendedSource };
        case Execution::Args::Type::CustomHeader:
            return { type, "header"_liv, ArgTypeCategory::ExtendedSource };
        case Execution::Args::Type::AcceptSourceAgreements:
            return { type, "accept-source-agreements"_liv, ArgTypeCategory::ExtendedSource };

        case Execution::Args::Type::Proxy:
            return { type, "proxy"_liv, ArgTypeCategory::CopyValueToSubContext, ArgTypeExclusiveSet::Proxy };
        case Execution::Args::Type::NoProxy:
            return { type, "no-proxy"_liv, ArgTypeCategory::CopyFlagToSubContext, ArgTypeExclusiveSet::Proxy };

        case Execution::Args::Type::ToolVersion:
            return { type, "version"_liv, 'v' };

        // Authentication arguments
        case Execution::Args::Type::AuthenticationMode:
            return { type, "authentication-mode"_liv, ArgTypeCategory::CopyValueToSubContext };
        case Execution::Args::Type::AuthenticationAccount:
            return { type, "authentication-account"_liv, ArgTypeCategory::CopyValueToSubContext };

        // Used for demonstration purposes
        case Execution::Args::Type::ExperimentalArg:
                return { type, "arg"_liv };

        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<ArgumentCommon> ArgumentCommon::GetFromExecArgs(const Execution::Args& execArgs)
    {
        auto argTypes = execArgs.GetTypes();
        std::vector<ArgumentCommon> result;
        std::transform(argTypes.begin(), argTypes.end(), std::back_inserter(result), ArgumentCommon::ForType);
        return result;
    }

    Argument Argument::ForType(Execution::Args::Type type)
    {
        switch (type)
        {
        case Args::Type::Query:
            return Argument{ type, Resource::String::QueryArgumentDescription, ArgumentType::Positional};
        case Args::Type::MultiQuery:
            return Argument{ type, Resource::String::MultiQueryArgumentDescription, ArgumentType::Positional }.SetCountLimit(128);
        case Args::Type::Manifest:
            return Argument{ type, Resource::String::ManifestArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help, Settings::TogglePolicy::Policy::LocalManifestFiles, Settings::BoolAdminSetting::LocalManifestFiles };
        case Args::Type::Id:
            return Argument{ type, Resource::String::IdArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Name:
            return Argument{ type, Resource::String::NameArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Moniker:
            return Argument{ type, Resource::String::MonikerArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Tag:
            return Argument{ type, Resource::String::TagArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Command:
            return Argument{ type, Resource::String::CommandArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Source:
            return Argument{ type, Resource::String::SourceArgumentDescription, ArgumentType::Standard };
        case Args::Type::DependencySource:
            return Argument{ type, Resource::String::DependencySourceArgumentDescription, ArgumentType::Standard };
        case Args::Type::Count:
            return Argument{ type, Resource::String::CountArgumentDescription, ArgumentType::Standard };
        case Args::Type::Exact:
            return Argument{ type, Resource::String::ExactArgumentDescription, ArgumentType::Flag };
        case Args::Type::Version:
            return Argument{ type, Resource::String::VersionArgumentDescription, ArgumentType::Standard };
        case Args::Type::Channel:
            return Argument{ type, Resource::String::ChannelArgumentDescription, ArgumentType::Standard, Argument::Visibility::Hidden };
        case Args::Type::Interactive:
            return Argument{ type, Resource::String::InteractiveArgumentDescription, ArgumentType::Flag };
        case Args::Type::Silent:
            return Argument{ type, Resource::String::SilentArgumentDescription, ArgumentType::Flag };
        case Args::Type::Locale:
            return Argument{ type, Resource::String::LocaleArgumentDescription, ArgumentType::Standard };
        case Args::Type::InstallArchitecture:
            return Argument{ type, Resource::String::ArchitectureArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::InstallerArchitecture:
            return Argument{ type, Resource::String::ArchitectureArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Log:
            return Argument{ type, Resource::String::LogArgumentDescription, ArgumentType::Standard };
        case Args::Type::CustomSwitches:
            return Argument{ type, Resource::String::CustomSwitchesArgumentDescription, ArgumentType::Standard };
        case Args::Type::Override:
            return Argument{ type, Resource::String::OverrideArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::InstallLocation:
            return Argument{ type, Resource::String::LocationArgumentDescription, ArgumentType::Standard };
        case Args::Type::HashOverride:
            return Argument{ type, Resource::String::HashOverrideArgumentDescription, ArgumentType::Flag, Settings::TogglePolicy::Policy::HashOverride, Settings::BoolAdminSetting::InstallerHashOverride };
        case Args::Type::AcceptPackageAgreements:
            return Argument{ type, Resource::String::AcceptPackageAgreementsArgumentDescription, ArgumentType::Flag };
        case Args::Type::NoUpgrade:
            return Argument{ type, Resource::String::NoUpgradeArgumentDescription, ArgumentType::Flag };
        case Args::Type::HashFile:
            return Argument{ type, Resource::String::FileArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::Msix:
            return Argument{ type, Resource::String::MsixArgumentDescription, ArgumentType::Flag };
        case Args::Type::ListVersions:
            return Argument{ type, Resource::String::VersionsArgumentDescription, ArgumentType::Flag };
        case Args::Type::Help:
            return Argument{ type, Resource::String::HelpArgumentDescription, ArgumentType::Flag };
        case Args::Type::SkipDependencies:
            return Argument{ type, Resource::String::SkipDependenciesArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::IgnoreLocalArchiveMalwareScan:
            return Argument{ type, Resource::String::IgnoreLocalArchiveMalwareScanArgumentDescription, ArgumentType::Flag, Settings::TogglePolicy::Policy::LocalArchiveMalwareScanOverride, Settings::BoolAdminSetting::LocalArchiveMalwareScanOverride };
        case Args::Type::SourceName:
            return Argument{ type, Resource::String::SourceNameArgumentDescription, ArgumentType::Positional, false };
        case Args::Type::SourceArg:
            return Argument{ type, Resource::String::SourceArgArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::SourceType:
            return Argument{ type, Resource::String::SourceTypeArgumentDescription, ArgumentType::Positional };
        case Args::Type::SourceExplicit:
            return Argument{ type, Resource::String::SourceExplicitArgumentDescription, ArgumentType::Flag };
        case Args::Type::SourceTrustLevel:
            return Argument{ type, Resource::String::SourceTrustLevelArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::ValidateManifest:
            return Argument{ type, Resource::String::ValidateManifestArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::IgnoreWarnings:
            return Argument{ type, Resource::String::IgnoreWarningsArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help };
        case Args::Type::NoVT:
            return Argument{ type, Resource::String::NoVTArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::RainbowStyle:
            return Argument{ type, Resource::String::RainbowArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::RetroStyle:
            return Argument{ type, Resource::String::RetroArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::VerboseLogs:
            return Argument{ type, Resource::String::VerboseLogsArgumentDescription, ArgumentType::Flag };
        case Args::Type::CustomHeader:
            return Argument{ type, Resource::String::HeaderArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::AcceptSourceAgreements:
            return Argument{ type, Resource::String::AcceptSourceAgreementsArgumentDescription, ArgumentType::Flag };
        case Args::Type::AuthenticationMode:
            return Argument{ type, Resource::String::AuthenticationModeArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::AuthenticationAccount:
            return Argument{ type, Resource::String::AuthenticationAccountArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::ExperimentalArg:
            return Argument{ type, Resource::String::ExperimentalArgumentDescription, ArgumentType::Flag, ExperimentalFeature::Feature::ExperimentalArg };
        case Args::Type::Rename:
            return Argument{ type, Resource::String::RenameArgumentDescription, ArgumentType::Standard, false };
        case Args::Type::Purge:
            return Argument{ type, Resource::String::PurgeArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::Preserve:
            return Argument{ type, Resource::String::PreserveArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::Wait:
            return Argument{ type, Resource::String::WaitArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::ProductCode:
            return Argument{ type, Resource::String::ProductCodeArgumentDescription, ArgumentType::Standard, false };
        case Args::Type::OpenLogs:
            return Argument{ type, Resource::String::OpenLogsArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help };
        case Args::Type::UninstallPrevious:
            return Argument{ type, Resource::String::UninstallPreviousArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help };
        case Args::Type::Force:
            return Argument{ type, Resource::String::ForceArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::DownloadDirectory:
            return Argument{ type, Resource::String::DownloadDirectoryArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help, false };
        case Args::Type::SkipMicrosoftStorePackageLicense:
            return Argument{ type, Resource::String::SkipMicrosoftStorePackageLicenseArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help, false };
        case Args::Type::Platform:
            return Argument{ type, Resource::String::PlatformArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help, false };
        case Args::Type::InstallerType:
            return Argument{ type, Resource::String::InstallerTypeArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help, false };
        case Args::Type::ResumeId:
            return Argument{ type, Resource::String::ResumeIdArgumentDescription, ArgumentType::Standard, true };
        case Args::Type::AllowReboot:
            return Argument{ type, Resource::String::AllowRebootArgumentDescription, ArgumentType::Flag };
        case Args::Type::IgnoreResumeLimit:
            return Argument{ type, Resource::String::IgnoreResumeLimitArgumentDescription, ArgumentType::Flag, ExperimentalFeature::Feature::Resume };
        case Args::Type::AllVersions:
            return Argument{ type, Resource::String::UninstallAllVersionsArgumentDescription, ArgumentType::Flag, Argument::Visibility::Help };
        case Args::Type::TargetVersion:
            return Argument{ type, Resource::String::TargetVersionArgumentDescription, ArgumentType::Standard };
        case Args::Type::Proxy:
            return Argument{ type, Resource::String::ProxyArgumentDescription, ArgumentType::Standard, TogglePolicy::Policy::ProxyCommandLineOptions, BoolAdminSetting::ProxyCommandLineOptions };
        case Args::Type::NoProxy:
            return Argument{ type, Resource::String::NoProxyArgumentDescription, ArgumentType::Flag, TogglePolicy::Policy::ProxyCommandLineOptions, BoolAdminSetting::ProxyCommandLineOptions };
        case Args::Type::Family:
            return Argument{ type, Resource::String::FontFamilyNameArgumentDescription, ArgumentType::Positional, false };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    void Argument::GetCommon(std::vector<Argument>& args)
    {
        args.push_back(ForType(Args::Type::Help));
        args.push_back(ForType(Args::Type::Wait));
        args.push_back(ForType(Args::Type::OpenLogs));
        args.push_back(ForType(Args::Type::NoVT));
        args.push_back(ForType(Args::Type::RainbowStyle));
        args.push_back(ForType(Args::Type::RetroStyle));
        args.push_back(ForType(Args::Type::VerboseLogs));
        args.push_back(ForType(Args::Type::IgnoreWarnings));
        args.emplace_back(Args::Type::DisableInteractivity, Resource::String::DisableInteractivityArgumentDescription, ArgumentType::Flag, false);
        args.push_back(ForType(Args::Type::Proxy));
        args.push_back(ForType(Args::Type::NoProxy));
    }

    std::string Argument::GetUsageString() const
    {
        std::ostringstream strstr;
        if (Alias() != ArgumentCommon::NoAlias)
        {
            strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << Alias() << ',';
        }
        if (AlternateName() != Argument::NoAlternateName)
        {
            strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << AlternateName() << ',';
        }
        strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << Name();
        return strstr.str();
    }

    void Argument::ValidateExclusiveArguments(const Execution::Args& args)
    {
        auto argProperties = ArgumentCommon::GetFromExecArgs(args);

        using ExclusiveSet_t = std::underlying_type_t<ArgTypeExclusiveSet>;
        for (ExclusiveSet_t i = 1 + static_cast<ExclusiveSet_t>(ArgTypeExclusiveSet::None); i < static_cast<ExclusiveSet_t>(ArgTypeExclusiveSet::Max); i <<= 1)
        {
            std::vector<ArgumentCommon> argsFromSet;
            std::copy_if(
                argProperties.begin(),
                argProperties.end(),
                std::back_inserter(argsFromSet),
                [=](const ArgumentCommon& arg) { return static_cast<ExclusiveSet_t>(arg.ExclusiveSet) & i; });

            if (argsFromSet.size() > 1)
            {
                // Create a string showing the exclusive args.
                std::string argsString;
                for (const auto& arg : argsFromSet)
                {
                    if (!argsString.empty())
                    {
                        argsString += '|';

                    }

                    argsString += arg.Name;
                }

                throw CommandException(Resource::String::MultipleExclusiveArgumentsProvided(Utility::LocIndString{ argsString }));
            }
        }
    }

    void Argument::ValidateArgumentDependency(const Execution::Args& args, Execution::Args::Type type, Execution::Args::Type dependencyArgType)
    {
        if (args.Contains(type) && !args.Contains(dependencyArgType))
        {
            throw CommandException(Resource::String::DependencyArgumentMissing(
                Utility::LocIndString{ ArgumentCommon::ForType(type).Name },
                Utility::LocIndString{ ArgumentCommon::ForType(dependencyArgType).Name }));
        }
    }

    ArgTypeCategory Argument::GetCategoriesPresent(const Execution::Args& args)
    {
        auto argProperties = ArgumentCommon::GetFromExecArgs(args);

        ArgTypeCategory result = ArgTypeCategory::None;
        for (const auto& arg : argProperties)
        {
            result |= arg.TypeCategory;
        }

        return result;
    }

    ArgTypeCategory Argument::GetCategoriesAndValidateCommonArguments(const Execution::Args& args, bool requirePackageSelectionArg)
    {
        const auto categories = GetCategoriesPresent(args);

        // Commands like install require some argument to select a package
        if (requirePackageSelectionArg)
        {
            if (WI_AreAllFlagsClear(categories, ArgTypeCategory::Manifest | ArgTypeCategory::PackageQuery | ArgTypeCategory::SinglePackageQuery))
            {
                throw CommandException(Resource::String::NoPackageSelectionArgumentProvided);
            }
        }

        // If a manifest is specified, we cannot also have arguments for searching
        if (WI_IsFlagSet(categories, ArgTypeCategory::Manifest) &&
            WI_IsAnyFlagSet(categories, ArgTypeCategory::PackageQuery | ArgTypeCategory::QuerySource))
        {
            throw CommandException(Resource::String::BothManifestAndSearchQueryProvided);
        }

        // If we have multiple packages, we cannot have arguments that only make sense for a single package
        if (WI_IsFlagSet(categories, ArgTypeCategory::MultiplePackages) &&
            WI_IsAnyFlagSet(categories, ArgTypeCategory::SinglePackageQuery | ArgTypeCategory::SingleInstallerBehavior))
        {
            throw CommandException(Resource::String::ArgumentForSinglePackageProvidedWithMultipleQueries);
        }

        return categories;
    }

    Argument::Visibility Argument::GetVisibility() const
    {
        if (!ExperimentalFeature::IsEnabled(m_feature))
        {
            return Argument::Visibility::Hidden;
        }

        if (!GroupPolicies().IsEnabled(m_groupPolicy))
        {
            return Argument::Visibility::Hidden;
        }

        return m_visibility;
    }
}
