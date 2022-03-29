// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <string_view>
#include <map>
#include <vector>

namespace AppInstaller::CLI::Execution
{
    struct Args
    {
        enum class Type
        {
            // Args to specify where to get app
            Query, // Query to be performed against index
            Manifest, // Provide the app manifest directly

            // Query filtering criteria and query behavior
            Id,
            Name,
            Moniker,
            Tag,
            Command,
            Source, // Index source to be queried against
            Count, // Maximum query results
            Exact, // Exact match required

            // Manifest selection behavior after an app is found
            Version,
            Channel,

            // Install behavior
            Interactive,
            Silent,
            Locale,
            Log,
            Override, //Override args are (and the only args) directly passed to installer
            InstallLocation,
            InstallScope,
            InstallArchitecture,
            HashOverride, // Ignore hash mismatches
            AcceptPackageAgreements, // Accept all license agreements for packages
            Rename, // Renames the file of the executable. Only applies to the portable installerType

            // Uninstall behavior
            Purge, // Removes all files and directories related to a package during an uninstall. Only applies to the portable installerType.
            Preserve, // Retains any files and directories created by the portable exe.

            //Source Command
            SourceName,
            SourceType,
            SourceArg,
            ForceSourceReset,

            //Hash Command
            HashFile,
            Msix, // Flag to indicate the input file is msix

            //Validate Command
            ValidateManifest,

            // Complete Command
            Word,
            CommandLine,
            Position,

            // Export Command
            OutputFile,
            IncludeVersions,

            // Import Command
            ImportFile,
            IgnoreUnavailable,
            IgnoreVersions,

            // Setting Command
            AdminSettingEnable,
            AdminSettingDisable,

            // Other
            All, // Used in Update command to update all installed packages to latest
            ListVersions, // Used in Show command to list all available versions of an app
            NoVT, // Disable VirtualTerminal outputs
            RetroStyle, // Makes progress display as retro
            RainbowStyle, // Makes progress display as a rainbow
            Help, // Show command usage
            Info, // Show general info about WinGet
            VerboseLogs, // Increases winget logging level to verbose
            DependencySource, // Index source to be queried against for finding dependencies
            CustomHeader, // Optional Rest source header
            AcceptSourceAgreements, // Accept all source agreements
            IncludeUnknown, // Used in Upgrade command to allow upgrades of packages with unknown versions
            Wait, // Prompts the user to press any key before exiting.

            // Used for demonstration purposes
            ExperimentalArg,
        };

        bool Contains(Type arg) const { return (m_parsedArgs.count(arg) != 0); }

        const std::vector<std::string>* GetArgs(Type arg) const
        {
            auto itr = m_parsedArgs.find(arg);
            return (itr == m_parsedArgs.end() ? nullptr : &(itr->second));
        }

        std::string_view GetArg(Type arg) const
        {
            auto itr = m_parsedArgs.find(arg);

            if (itr == m_parsedArgs.end())
            {
                return {};
            }

            return itr->second[0];
        }

        size_t GetCount(Type arg) const
        {
            auto args = GetArgs(arg);
            return (args ? args->size() : 0);
        }

        bool AddArg(Type arg)
        {
            return m_parsedArgs[arg].empty();
        }

        void AddArg(Type arg, std::string value)
        {
            m_parsedArgs[arg].emplace_back(std::move(value));
        }

        void AddArg(Type arg, std::string_view value)
        {
            m_parsedArgs[arg].emplace_back(value);
        }

        bool Empty()
        {
            return m_parsedArgs.empty();
        }

        size_t GetArgsCount()
        {
            return m_parsedArgs.size();
        }

        std::vector<Type> GetTypes()
        {
            std::vector<Type> types;

            for (auto const& i : m_parsedArgs)
            {
                types.emplace_back(i.first);
            }

            return types;
        }

    private:
        std::map<Type, std::vector<std::string>> m_parsedArgs;
    };
}
