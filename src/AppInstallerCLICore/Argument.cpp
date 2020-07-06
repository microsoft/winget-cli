// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Argument.h"
#include "Resources.h"
#include <winget/UserSettings.h>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace Settings;

    Argument Argument::ForType(Execution::Args::Type type)
    {
        switch (type)
        {
        case Args::Type::Query:
            return Argument{ "query", 'q', Args::Type::Query, Resource::String::QueryArgumentDescription, ArgumentType::Positional};
        case Args::Type::Manifest:
            return Argument{ "manifest", 'm', Args::Type::Manifest, Resource::String::ManifestArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Id:
            return Argument{ "id", NoAlias, Args::Type::Id,Resource::String::IdArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Name:
            return Argument{ "name", NoAlias, Args::Type::Name, Resource::String::NameArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Moniker:
            return Argument{ "moniker", NoAlias, Args::Type::Moniker, Resource::String::MonikerArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Tag:
            return Argument{ "tag", NoAlias, Args::Type::Tag, Resource::String::TagArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Command:
            return Argument{ "command", NoAlias, Args::Type::Command, Resource::String::CommandArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::Source:
            return Argument{ "source", 's', Args::Type::Source, Resource::String::SourceArgumentDescription, ArgumentType::Standard };
        case Args::Type::Count:
            return Argument{ "count", 'n', Args::Type::Count, Resource::String::CountArgumentDescription, ArgumentType::Standard };
        case Args::Type::Exact:
            return Argument{ "exact", 'e', Args::Type::Exact, Resource::String::ExactArgumentDescription, ArgumentType::Flag };
        case Args::Type::Version:
            return Argument{ "version", 'v', Args::Type::Version, Resource::String::VersionArgumentDescription, ArgumentType::Standard };
        case Args::Type::Channel:
            return Argument{ "channel", 'c', Args::Type::Channel, Resource::String::ChannelArgumentDescription, ArgumentType::Standard, Argument::Visibility::Hidden };
        case Args::Type::Interactive:
            return Argument{ "interactive", 'i', Args::Type::Interactive, Resource::String::InteractiveArgumentDescription, ArgumentType::Flag };
        case Args::Type::Silent:
            return Argument{ "silent", 'h', Args::Type::Silent, Resource::String::SilentArgumentDescription, ArgumentType::Flag };
        case Args::Type::Language:
            return Argument{ "lang", 'a', Args::Type::Language, Resource::String::LanguageArgumentDescription, ArgumentType::Standard, Argument::Visibility::Hidden };
        case Args::Type::Log:
            return Argument{ "log", 'o', Args::Type::Log, Resource::String::LogArgumentDescription, ArgumentType::Standard };
        case Args::Type::Override:
            return Argument{ "override", NoAlias, Args::Type::Override, Resource::String::OverrideArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help };
        case Args::Type::InstallLocation:
            return Argument{ "location", 'l', Args::Type::InstallLocation, Resource::String::LocationArgumentDescription, ArgumentType::Standard };
        case Args::Type::HashFile:
            return Argument{ "file", 'f', Args::Type::HashFile, Resource::String::FileArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::Msix:
            return Argument{ "msix", 'm', Args::Type::Msix, Resource::String::MsixArgumentDescription, ArgumentType::Flag };
        case Args::Type::ListVersions:
            return Argument{ "versions", NoAlias, Args::Type::ListVersions, Resource::String::VersionsArgumentDescription, ArgumentType::Flag };
        case Args::Type::Help:
            return Argument{ "help", APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR, Args::Type::Help, Resource::String::HelpArgumentDescription, ArgumentType::Flag };
        case Args::Type::SourceName:
            return Argument{ "name", 'n', Args::Type::SourceName,Resource::String::SourceNameArgumentDescription, ArgumentType::Positional, false };
        case Args::Type::SourceArg:
            return Argument{ "arg", 'a', Args::Type::SourceArg, Resource::String::SourceArgArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::SourceType:
            return Argument{ "type", 't', Args::Type::SourceType, Resource::String::SourceTypeArgumentDescription, ArgumentType::Positional };
        case Args::Type::ValidateManifest:
            return Argument{ "manifest", NoAlias, Args::Type::ValidateManifest, Resource::String::ValidateManifestArgumentDescription, ArgumentType::Positional, true };
        case Args::Type::NoVT:
            return Argument{ "no-vt", NoAlias, Args::Type::NoVT, Resource::String::NoVTArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::RainbowStyle:
            return Argument{ "rainbow", NoAlias, Args::Type::RainbowStyle, Resource::String::RainbowArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::RetroStyle:
            return Argument{ "retro", NoAlias, Args::Type::RetroStyle, Resource::String::RetroArgumentDescription, ArgumentType::Flag, Argument::Visibility::Hidden };
        case Args::Type::VerboseLogs:
            return Argument{ "verbose-logs", NoAlias, Args::Type::VerboseLogs, Resource::String::VerboseLogsArgumentDescription, ArgumentType::Flag };
        case Args::Type::ExperimentalArg:
            return Argument{ "arg", NoAlias, Args::Type::ExperimentalArg, Resource::String::ExperimentalArgumentDescription, ArgumentType::Flag, ExperimentalFeature::Feature::ExperimentalArg };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    void Argument::GetCommon(std::vector<Argument>& args)
    {
        args.push_back(ForType(Args::Type::Help));
        args.push_back(ForType(Args::Type::NoVT));
        args.push_back(ForType(Args::Type::RainbowStyle));
        args.push_back(ForType(Args::Type::RetroStyle));
        args.push_back(ForType(Args::Type::VerboseLogs));
    }

    Argument::Visibility Argument::GetVisibility() const
    {
        if (!ExperimentalFeature::IsEnabled(m_feature))
        {
            return Argument::Visibility::Hidden;
        }

        return m_visibility;
    }
}
