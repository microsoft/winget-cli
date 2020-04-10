// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Argument.h"
#include "Localization.h"


namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;

    Argument Argument::ForType(Execution::Args::Type type)
    {
        constexpr char None = APPINSTALLER_CLI_ARGUMENT_NO_SHORT_VER;

        switch (type)
        {
        case Args::Type::Query:
            return Argument{ "query", 'q', Args::Type::Query, LOCME("The query used to search for an app"), ArgumentType::Positional };
        case Args::Type::Manifest:
            return Argument{ "manifest", 'm', Args::Type::Manifest, LOCME("The path to the manifest of the application to install"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Id:
            return Argument{ "id", None, Args::Type::Id, LOCME("Filter results by id"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Name:
            return Argument{ "name", None, Args::Type::Name, LOCME("Filter results by name"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Moniker:
            return Argument{ "moniker", None, Args::Type::Moniker, LOCME("Filter results by app moniker"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Tag:
            return Argument{ "tag", None, Args::Type::Tag, LOCME("Filter results by tag"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Command:
            return Argument{ "command", None, Args::Type::Command, LOCME("Filter results by command"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::Source:
            return Argument{ "source", 's', Args::Type::Source, LOCME("Find app using the specified source"), ArgumentType::Standard };
        case Args::Type::Count:
            return Argument{ "count", 'n', Args::Type::Count, LOCME("Show no more than specified number of results"), ArgumentType::Standard };
        case Args::Type::Exact:
            return Argument{ "exact", 'e', Args::Type::Exact, LOCME("Find app using exact match"), ArgumentType::Flag };
        case Args::Type::Version:
            return Argument{ "version", 'v', Args::Type::Version, LOCME("Use the specified version; default is the latest version"), ArgumentType::Standard };
        case Args::Type::Channel:
            return Argument{ "channel", 'c', Args::Type::Channel, LOCME("Use the specified channel; default is general audience"), ArgumentType::Standard, Visibility::Hidden };
        case Args::Type::Interactive:
            return Argument{ "interactive", 'i', Args::Type::Interactive, LOCME("Request interactive installation; user input may be needed"), ArgumentType::Flag };
        case Args::Type::Silent:
            return Argument{ "silent", 'h', Args::Type::Silent, LOCME("Request silent installation"), ArgumentType::Flag };
        case Args::Type::Language:
            return Argument{ "lang", 'a', Args::Type::Language, LOCME("Language to install (if supported)"), ArgumentType::Standard, Visibility::Hidden };
        case Args::Type::Log:
            return Argument{ "log", 'o', Args::Type::Log, LOCME("Log location (if supported)"), ArgumentType::Standard };
        case Args::Type::Override:
            return Argument{ "override", None, Args::Type::Override, LOCME("Override arguments to be passed on to the installer"), ArgumentType::Standard, Visibility::Help };
        case Args::Type::InstallLocation:
            return Argument{ "location", 'l', Args::Type::InstallLocation, LOCME("Location to install to (if supported)"), ArgumentType::Standard };
        case Args::Type::HashFile:
            return Argument{ "file", 'f', Args::Type::HashFile, LOCME("File to be hashed"), ArgumentType::Positional, true };
        case Args::Type::Msix:
            return Argument{ "msix", 'm', Args::Type::Msix, LOCME("Input file will be treated as msix; signature hash will be provided if signed"), ArgumentType::Flag };
        case Args::Type::ListVersions:
            return Argument{ "versions", None, Args::Type::ListVersions, LOCME("Show available versions of the app"), ArgumentType::Flag };
        case Args::Type::Help:
            return Argument{ "help", APPINSTALLER_CLI_HELP_ARGUMENT_TEXT_CHAR, Args::Type::Help, LOCME("Shows help about the selected command"), ArgumentType::Flag };
        case Args::Type::SourceName:
            return Argument{ "name", 'n', Args::Type::SourceName, LOCME("Name of the source"), ArgumentType::Positional, false };
        case Args::Type::SourceArg:
            return Argument{ "arg", 'a', Args::Type::SourceArg, LOCME("Argument given to the source"), ArgumentType::Positional, true };
        case Args::Type::SourceType:
            return Argument{ "type", 't', Args::Type::SourceType, LOCME("Type of the source"), ArgumentType::Positional };
        case Args::Type::ValidateManifest:
            return Argument{ "manifest", None, Args::Type::ValidateManifest, LOCME("The path to the manifest to be validated"), ArgumentType::Positional, true };
        case Args::Type::NoVT:
            return Argument{ "no-vt", None, Args::Type::NoVT, LOCME("Disables VirtualTerminal display"), ArgumentType::Flag, Visibility::Hidden };
        case Args::Type::RainbowProgress:
            return Argument{ "rainbow", None, Args::Type::RainbowProgress, LOCME("Progress bars display a rainbow of colors"), ArgumentType::Flag, Visibility::Hidden };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    void Argument::GetCommon(std::vector<Argument>& args)
    {
        args.push_back(ForType(Args::Type::Help));
        args.push_back(ForType(Args::Type::NoVT));
        args.push_back(ForType(Args::Type::RainbowProgress));
    }
}
