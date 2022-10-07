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

    Argument Argument::ForType(Execution::Args::Type type)
    {
        switch (type)
        {
        case Args::Type::Query:
            return Argument{ "query"_liv, 'q', Args::Type::Query, Resource::String::QueryArgumentDescription, ArgumentType::Positional};
        case Args::Type::Manifest:
            return Argument{ "manifest"_liv, 'm', Args::Type::Manifest, Resource::String::ManifestArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help, Settings::TogglePolicy::Policy::LocalManifestFiles, Settings::AdminSetting::LocalManifestFiles };
        case Args::Type::Id:
            return Argument{ "id"_liv, NoAlias, Args::Type::Id,Resource::String::IdArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Name:
            return Argument{ "name"_liv, NoAlias, Args::Type::Name, Resource::String::NameArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Moniker:
            return Argument{ "moniker"_liv, NoAlias, Args::Type::Moniker, Resource::String::MonikerArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Tag:
            return Argument{ "tag"_liv, NoAlias, Args::Type::Tag, Resource::String::TagArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Command:
            return Argument{ "command"_liv, NoAlias, "cmd"_liv, Args::Type::Command, Resource::String::CommandArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Source:
            return Argument{ "source"_liv, 's', Args::Type::Source, Resource::String::SourceArgumentDescription, ArgumentType::Standard };
        case Args::Type::DependencySource:
            return Argument{ "dependency-source"_liv, NoAlias, Args::Type::DependencySource, Resource::String::DependencySourceArgumentDescription, ArgumentType::Standard };
        case Args::Type::Count:
            return Argument{ "count"_liv, 'n', Args::Type::Count, Resource::String::CountArgumentDescription, ArgumentType::Standard };
        case Args::Type::Exact:
            return Argument{ "exact"_liv, 'e', Args::Type::Exact, Resource::String::ExactArgumentDescription, ArgumentType::Flag };
        case Args::Type::Version:
            return Argument{ "version"_liv, 'v', Args::Type::Version, Resource::String::VersionArgumentDescription, ArgumentType::Standard };
        case Args::Type::Channel:
            return Argument{ "channel"_liv, 'c', Args::Type::Channel, Resource::String::ChannelArgumentDescription, ArgumentType::Standard, Argument::Visibility::Hidden };
        case Args::Type::Interactive:
            return Argument{ "interactive"_liv, 'i', Args::Type::Interactive, Resource::String::InteractiveArgumentDescription, ArgumentType::Flag };
        case Args::Type::Silent:
            return Argument{ "silent"_liv, 'h', Args::Type::Silent, Resource::String::SilentArgumentDescription, ArgumentType::Flag };
        case Args::Type::Locale:
            return Argument{ "locale"_liv, NoAlias, Args::Type::Locale, Resource::String::LocaleArgumentDescription, ArgumentType::Standard };
        case Args::Type::InstallArchitecture:
            return Argument{ "architecture"_liv, 'a', Args::Type::InstallArchitecture, Resource::String::InstallArchitectureArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Log:
            return Argument{ "log"_liv, 'o', Args::Type::Log, Resource::String::LogArgumentDescription, ArgumentType::Standard };
        case Args::Type::Override:
            return Argument{ "override"_liv, NoAlias, Args::Type::Override, Resource::String::OverrideArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::InstallLocation:
            return Argument{ "location"_liv, 'l', Args::Type::InstallLocation, Resource::String::LocationArgumentDescription, ArgumentType::Standard };
        case Args::Type::HashOverride:
            return Argument{ "force"_liv, NoAlias, Args::Type::HashOverride, Resource::String::InstallForceArgumentDescription, ArgumentType::Flag, Settings::TogglePolicy::Policy::HashOverride };
        case Args::Type::AcceptPackageAgreements:
            return Argument{ "accept-package-agreements"_liv, NoAlias, Args::Type::AcceptPackageAgreements, Resource::String::AcceptPackageAgreementsArgumentDescription, ArgumentType::Flag };
        case Args::Type::HashFile:
            return Argument{ "file"_liv, 'f', Args::Type::HashFile, Resource::String::FileArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::Msix:
            return Argument{ "msix"_liv, 'm', Args::Type::Msix, Resource::String::MsixArgumentDescription, ArgumentType::Flag };
        case Args::Type::ListVersions:
            return Argument{ "versions"_liv, NoAlias, Args::Type::ListVersions, Resource::String::VersionsArgumentDescription, ArgumentType::Flag };
        case Args::Type::Help:
            return Argument{ "help"_liv, APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR, Args::Type::Help, Resource::String::HelpArgumentDescription, ArgumentType::Flag };
        case Args::Type::SourceName:
            return Argument{ "name"_liv, 'n', Args::Type::SourceName,Resource::String::SourceNameArgumentDescription, ArgumentType::Positional, false };
        case Args::Type::SourceArg:
            return Argument{ "arg"_liv, 'a', Args::Type::SourceArg, Resource::String::SourceArgArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::SourceType:
            return Argument{ "type"_liv, 't', Args::Type::SourceType, Resource::String::SourceTypeArgumentDescription, ArgumentType::Positional };
        case Args::Type::ValidateManifest:
            return Argument{ "manifest"_liv, NoAlias, Args::Type::ValidateManifest, Resource::String::ValidateManifestArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::NoVT:
            return Argument{ "no-vt"_liv, NoAlias, Args::Type::NoVT, Resource::String::NoVTArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::RainbowStyle:
            return Argument{ "rainbow"_liv, NoAlias, Args::Type::RainbowStyle, Resource::String::RainbowArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::RetroStyle:
            return Argument{ "retro"_liv, NoAlias, Args::Type::RetroStyle, Resource::String::RetroArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::VerboseLogs:
            return Argument{ "verbose-logs"_liv, NoAlias, "verbose"_liv, Args::Type::VerboseLogs, Resource::String::VerboseLogsArgumentDescription, ArgumentType::Flag};
        case Args::Type::CustomHeader:
            return Argument{ "header"_liv, NoAlias, Args::Type::CustomHeader, Resource::String::HeaderArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::AcceptSourceAgreements:
            return Argument{ "accept-source-agreements"_liv, NoAlias, Args::Type::AcceptSourceAgreements, Resource::String::AcceptSourceAgreementsArgumentDescription, ArgumentType::Flag };
        case Args::Type::ExperimentalArg:
            return Argument{ "arg"_liv, NoAlias, Args::Type::ExperimentalArg, Resource::String::ExperimentalArgumentDescription, ArgumentType::Flag, ExperimentalFeature::Feature::ExperimentalArg };
        case Args::Type::Rename:
            return Argument{ "rename"_liv, 'r', Args::Type::Rename, Resource::String::RenameArgumentDescription, ArgumentType::Standard, false };
        case Args::Type::Purge:
            return Argument{ "purge"_liv, NoAlias, Args::Type::Purge, Resource::String::PurgeArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::Preserve:
            return Argument{ "preserve"_liv, NoAlias, Args::Type::Preserve, Resource::String::PreserveArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::Wait:
            return Argument{ "wait"_liv, NoAlias, Args::Type::Wait, Resource::String::WaitArgumentDescription, ArgumentType::Flag, false };
        case Args::Type::ProductCode:
            return Argument{ "product-code"_liv, NoAlias, Args::Type::ProductCode, Resource::String::ProductCodeArgumentDescription, ArgumentType::Standard, false };
        case Args::Type::OpenLogs:
            return Argument{ "open-logs"_liv, NoAlias, "logs"_liv, Args::Type::OpenLogs, Resource::String::OpenLogsArgumentDescription, ArgumentType::Flag, ExperimentalFeature::Feature::OpenLogsArgument};
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
        args.emplace_back("disable-interactivity", NoAlias, Args::Type::DisableInteractivity, Resource::String::DisableInteractivityArgumentDescription, ArgumentType::Flag, false);
    }

    std::string Argument::GetUsageString() const
    {
        std::ostringstream strstr;
        if (m_alias != Argument::NoAlias)
        {
            strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << m_alias << ',';
        }
        if (m_alternateName != Argument::NoAlternateName)
        {
            strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << m_alternateName << ',';
        }
        strstr << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << APPINSTALLER_CLI_ARGUMENT_IDENTIFIER_CHAR << m_name;
        return strstr.str();
    }

    void Argument::ValidatePackageSelectionArgumentSupplied(const Execution::Args& args)
    {
        for (Args::Type type : { Args::Type::Query, Args::Type::Manifest, Args::Type::Id, Args::Type::Name, Args::Type::Moniker, Args::Type::ProductCode, Args::Type::Tag, Args::Type::Command })
        {
            if (args.Contains(type))
            {
                return;
            }
        }

        throw CommandException(Resource::String::NoPackageSelectionArgumentProvided);
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
