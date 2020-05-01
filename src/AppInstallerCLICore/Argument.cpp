// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Argument.h"
#include "Localization.h"
#include "Resources.h"


namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;

    Argument Argument::ForType(Execution::Args::Type type)
    {
        constexpr char None = APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER;

        switch (type)
        {
        case Args::Type::Query:
            return Argument{ "query", 'q', Args::Type::Query, Resources::GetInstance().ResolveWingetString(L"QueryArgumentDescription").c_str(), ArgumentType::Positional };
        case Args::Type::Manifest:
            return Argument{ "manifest", 'm', Args::Type::Manifest, Resources::GetInstance().ResolveWingetString(L"ManifestArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Id:
            return Argument{ "id", None, Args::Type::Id,Resources::GetInstance().ResolveWingetString(L"IdArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Name:
            return Argument{ "name", None, Args::Type::Name, Resources::GetInstance().ResolveWingetString(L"NameArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Moniker:
            return Argument{ "moniker", None, Args::Type::Moniker, Resources::GetInstance().ResolveWingetString(L"MonikerArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Tag:
            return Argument{ "tag", None, Args::Type::Tag, Resources::GetInstance().ResolveWingetString(L"TagArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Command:
            return Argument{ "command", None, Args::Type::Command, Resources::GetInstance().ResolveWingetString(L"CommandArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Source:
            return Argument{ "source", 's', Args::Type::Source, Resources::GetInstance().ResolveWingetString(L"SourceArgumentDescription").c_str(), ArgumentType::Standard };
        case Args::Type::Count:
            return Argument{ "count", 'n', Args::Type::Count, Resources::GetInstance().ResolveWingetString(L"CountArgumentDescription").c_str(), ArgumentType::Standard };
        case Args::Type::Exact:
            return Argument{ "exact", 'e', Args::Type::Exact, Resources::GetInstance().ResolveWingetString(L"ExactArgumentDescription").c_str(), ArgumentType::Flag };
        case Args::Type::Version:
            return Argument{ "version", 'v', Args::Type::Version, Resources::GetInstance().ResolveWingetString(L"VersionArgumentDescription").c_str(), ArgumentType::Standard };
        case Args::Type::Channel:
            return Argument{ "channel", 'c', Args::Type::Channel, Resources::GetInstance().ResolveWingetString(L"ChannelArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Hidden };
        case Args::Type::Interactive:
            return Argument{ "interactive", 'i', Args::Type::Interactive, Resources::GetInstance().ResolveWingetString(L"InteractiveArgumentDescription").c_str(), ArgumentType::Flag };
        case Args::Type::Silent:
            return Argument{ "silent", 'h', Args::Type::Silent, Resources::GetInstance().ResolveWingetString(L"SilentArgumentDescription").c_str(), ArgumentType::Flag };
        case Args::Type::Language:
            return Argument{ "lang", 'a', Args::Type::Language, Resources::GetInstance().ResolveWingetString(L"LanguageArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Hidden };
        case Args::Type::Log:
            return Argument{ "log", 'o', Args::Type::Log, Resources::GetInstance().ResolveWingetString(L"LogArgumentDesciption").c_str(), ArgumentType::Standard };
        case Args::Type::Override:
            return Argument{ "override", None, Args::Type::Override, Resources::GetInstance().ResolveWingetString(L"OverrideArgumentDescription").c_str(), ArgumentType::Standard, Visibility::Help };
        case Args::Type::InstallLocation:
            return Argument{ "location", 'l', Args::Type::InstallLocation, Resources::GetInstance().ResolveWingetString(L"LocationArgumentDescription").c_str(), ArgumentType::Standard };
        case Args::Type::HashFile:
            return Argument{ "file", 'f', Args::Type::HashFile, Resources::GetInstance().ResolveWingetString(L"FileArgumentDescription").c_str(), ArgumentType::Positional, true };
        case Args::Type::Msix:
            return Argument{ "msix", 'm', Args::Type::Msix, Resources::GetInstance().ResolveWingetString(L"MsixArgumentDescription").c_str(), ArgumentType::Flag };
        case Args::Type::ListVersions:
            return Argument{ "versions", None, Args::Type::ListVersions, Resources::GetInstance().ResolveWingetString(L"VersionsArgumentDescription").c_str(), ArgumentType::Flag };
        case Args::Type::Help:
            return Argument{ "help", APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR, Args::Type::Help, Resources::GetInstance().ResolveWingetString(L"HelpArgumentDescription").c_str(), ArgumentType::Flag };
        case Args::Type::SourceName:
            return Argument{ "name", 'n', Args::Type::SourceName,Resources::GetInstance().ResolveWingetString(L"SourceNameArgumentDescription").c_str(), ArgumentType::Positional, false };
        case Args::Type::SourceArg:
            return Argument{ "arg", 'a', Args::Type::SourceArg, Resources::GetInstance().ResolveWingetString(L"SourceArgArgumentDescription").c_str(), ArgumentType::Positional, true };
        case Args::Type::SourceType:
            return Argument{ "type", 't', Args::Type::SourceType, Resources::GetInstance().ResolveWingetString(L"SourceTypeArgumentDescription").c_str(), ArgumentType::Positional };
        case Args::Type::ValidateManifest:
            return Argument{ "manifest", None, Args::Type::ValidateManifest, Resources::GetInstance().ResolveWingetString(L"ValidateManifestArgumentDescription").c_str(), ArgumentType::Positional, true };
        case Args::Type::NoVT:
            return Argument{ "no-vt", None, Args::Type::NoVT, Resources::GetInstance().ResolveWingetString(L"NoVTArguementDescription").c_str(), ArgumentType::Flag, Visibility::Hidden };
        case Args::Type::RainbowStyle:
            return Argument{ "rainbow", None, Args::Type::RainbowStyle, Resources::GetInstance().ResolveWingetString(L"RainbowArgumentDescription").c_str(), ArgumentType::Flag, Visibility::Hidden };
        case Args::Type::PlainStyle:
            return Argument{ "plain", None, Args::Type::PlainStyle, Resources::GetInstance().ResolveWingetString(L"PlainArgumentDescription").c_str(), ArgumentType::Flag, Visibility::Hidden };
        case Args::Type::Force:
            return Argument{ "force", None, Args::Type::Force, Resources::GetInstance().ResolveWingetString(L"ForceArgumentDescription").c_str(), ArgumentType::Flag };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    void Argument::GetCommon(std::vector<Argument>& args)
    {
        args.push_back(ForType(Args::Type::Help));
        args.push_back(ForType(Args::Type::NoVT));
        args.push_back(ForType(Args::Type::RainbowStyle));
        args.push_back(ForType(Args::Type::PlainStyle));
    }
}
