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
        enum class Type : uint32_t
        {
            // Args to specify where to get app
            Query, // Query to be performed against index
            MultiQuery, // Like query, but can take multiple values
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
            CustomSwitches, // CustomSwitches args are args passed to the installer in addition to any defined in the manifest
            Override, // Override args are (and the only args) directly passed to installer
            InstallLocation,
            InstallScope,
            InstallArchitecture,
            InstallerArchitecture,
            InstallerType,
            HashOverride, // Ignore hash mismatches
            SkipDependencies, // Skip dependencies
            IgnoreLocalArchiveMalwareScan, // Ignore the local malware scan on archive files
            AcceptPackageAgreements, // Accept all license agreements for packages
            Rename, // Renames the file of the executable. Only applies to the portable installerType
            NoUpgrade, // Install flow should not try to convert to upgrade flow upon finding existing installed version
            AllowReboot, // Allows the reboot flow to proceed if applicable

            // Uninstall behavior
            Purge, // Removes all files and directories related to a package during an uninstall. Only applies to the portable installerType.
            Preserve, // Retains any files and directories created by the portable exe.
            ProductCode, // Uninstalls using the product code as the identifier.
            AllVersions, // Uninstall all versions of the package
            TargetVersion, // The specific version to target

            //Source Command
            SourceName,
            SourceType,
            SourceArg,
            ForceSourceReset,
            SourceExplicit,
            SourceTrustLevel,

            //Hash Command
            HashFile,
            Msix, // Flag to indicate the input file is msix

            //Validate Command
            ValidateManifest,
            IgnoreWarnings,

            // Complete Command
            Word,
            CommandLine,
            Position,

            // Export Command
            IncludeVersions,

            // Import Command
            ImportFile,
            IgnoreUnavailable,
            IgnoreVersions,

            // Download Command
            DownloadDirectory,
            SkipMicrosoftStorePackageLicense,
            Platform,

            // Setting Command
            AdminSettingEnable,
            AdminSettingDisable,
            SettingName,
            SettingValue,

            // Upgrade command
            All, // Used in Update command to update all installed packages to latest
            IncludeUnknown, // Used in Upgrade command to allow upgrades of packages with unknown versions
            IncludePinned, // Used in Upgrade command to allow upgrades to pinned packages (only for pinning type of pins)
            UninstallPrevious, // Used in Upgrade command to override the default manifest behavior to UninstallPrevious

            // Show command
            ListVersions, // Used in Show command to list all available versions of an app

            // List Command
            Upgrade, // Used in List command to only show versions with upgrades

            // Pin command
            GatedVersion, // Differs from Version in that this supports wildcards
            BlockingPin,
            PinInstalled,

            // Error command
            ErrorInput,

            // Resume Command
            ResumeId,
            IgnoreResumeLimit,

            // Font Command
            Family,

            // Configuration
            ConfigurationFile,
            ConfigurationAcceptWarning,
            ConfigurationSuppressPrologue,
            ConfigurationEnable,
            ConfigurationDisable,
            ConfigurationModulePath,
            ConfigurationExportPackageId,
            ConfigurationExportModule,
            ConfigurationExportResource,
            ConfigurationExportAll,
            ConfigurationHistoryItem,
            ConfigurationHistoryRemove,
            ConfigurationStatusWatch,

            // Common arguments
            NoVT, // Disable VirtualTerminal outputs
            RetroStyle, // Makes progress display as retro
            RainbowStyle, // Makes progress display as a rainbow
            Help, // Show command usage
            Info, // Show general info about WinGet
            VerboseLogs, // Increases winget logging level to verbose
            DisableInteractivity, // Disable interactive prompts
            Wait, // Prompts the user to press any key before exiting
            OpenLogs, // Opens the default logs directory after executing the command
            Force, // Forces the execution of the workflow with non security related issues
            OutputFile,

            DependencySource, // Index source to be queried against for finding dependencies
            CustomHeader, // Optional Rest source header
            AcceptSourceAgreements, // Accept all source agreements

            AuthenticationMode, // Authentication mode (silent, silentPreferred or interactive)
            AuthenticationAccount, // Authentication account to be used

            // Network Behavior
            Proxy, // Set a proxy to use in this execution
            NoProxy, // Do not use the default proxy

            ToolVersion,

            // Used for demonstration purposes
            ExperimentalArg,

            // This should always be at the end
            Max
        };

        template<typename... T, std::enable_if_t<(... && std::is_same_v<T, Args::Type>), bool> = true>
        bool Contains(T... arg) const
        {
            return (... && (m_parsedArgs.count(arg) != 0));
        }

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

        size_t GetArgsCount() const
        {
            return m_parsedArgs.size();
        }

        std::vector<Type> GetTypes() const
        {
            std::vector<Type> types;

            for (auto const& i : m_parsedArgs)
            {
                types.emplace_back(i.first);
            }

            return types;
        }

        // If the user passes the same value multiple times inside a MultiQuery, operations will be repeated
        // Since there currently is not a way to include search options within a MultiQuery, processing duplicates
        // does not make sense within a single invocation
        void MakeMultiQueryContainUniqueValues()
        {
            auto itr = m_parsedArgs.find(Type::MultiQuery);
            
            // If there is not a value in MultiQuery, or there is only one value, it is presumed to be unique
            if (itr == m_parsedArgs.end() || itr->second.size() == 1)
            {
                return;
            }

            std::set<std::string> querySet;
            std::vector<std::string>& queryStrings = itr->second;

            queryStrings.erase(std::remove_if(queryStrings.begin(), queryStrings.end(), [&](const std::string value) { return !querySet.insert(value).second; }), queryStrings.end());
        }

        // If we get a single value for multi-query, we remove the argument and add it back as a single query.
        // This way the rest of the code can assume that if there is a MultiQuery we will always have multiple values,
        // and if there is a single one it will be in the Query type.
        void MoveMultiQueryToSingleQueryIfNeeded()
        {
            auto itr = m_parsedArgs.find(Type::MultiQuery);
            if (itr != m_parsedArgs.end() && itr->second.size() == 1)
            {
                // A test ensures that commands don't have both Query and MultiQuery arguments,
                // so if we had a MultiQuery value, we can be sure there is no Query value
                m_parsedArgs[Type::Query].emplace_back(std::move(itr->second[0]));
                m_parsedArgs.erase(itr);
            }
        }

    private:
        std::map<Type, std::vector<std::string>> m_parsedArgs;
    };
}
