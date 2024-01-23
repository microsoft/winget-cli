// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerStrings.h"
#include "AppInstallerSHA256.h"
#include "winget/Yaml.h"
#include "winget/ManifestYamlWriter.h"

namespace AppInstaller::Manifest::YamlWriter
{
    using namespace Utility::literals;

    namespace
    {
        constexpr std::string_view PackageIdentifier = "PackageIdentifier"sv;
        constexpr std::string_view PackageFamilyName = "PackageFamilyName"sv;
        constexpr std::string_view ProductCode = "ProductCode"sv;
        constexpr std::string_view Versions = "Versions"sv;
        constexpr std::string_view PackageVersion = "PackageVersion"sv;
        constexpr std::string_view Channel = "Channel"sv;
        constexpr std::string_view ManifestVersion = "ManifestVersion"sv;
        constexpr std::string_view ManifestType = "ManifestType"sv;

        // Installer
        constexpr std::string_view Installers = "Installers"sv;
        constexpr std::string_view InstallerIdentifier = "InstallerIdentifier"sv;
        constexpr std::string_view InstallerSha256 = "InstallerSha256"sv;
        constexpr std::string_view InstallerUrl = "InstallerUrl"sv;
        constexpr std::string_view Architecture = "Architecture"sv;
        constexpr std::string_view InstallerLocale = "InstallerLocale"sv;
        constexpr std::string_view Platform = "Platform"sv;
        constexpr std::string_view InstallerType = "InstallerType"sv;
        constexpr std::string_view Scope = "Scope"sv;
        constexpr std::string_view SignatureSha256 = "SignatureSha256"sv;
        constexpr std::string_view InstallModes = "InstallModes"sv;
        constexpr std::string_view MSStoreProductIdentifier = "MSStoreProductIdentifier"sv;
        constexpr std::string_view ReleaseDate = "ReleaseDate"sv;
        constexpr std::string_view InstallerAbortsTerminal = "InstallerAbortsTerminal"sv;
        constexpr std::string_view InstallLocationRequired = "InstallLocationRequired"sv;
        constexpr std::string_view RequireExplicitUpgrade = "RequireExplicitUpgrade"sv;
        constexpr std::string_view UnsupportedOSArchitectures = "UnsupportedOSArchitectures"sv;
        constexpr std::string_view AppsAndFeaturesEntries = "AppsAndFeaturesEntries"sv;
        constexpr std::string_view DisplayVersion = "DisplayVersion"sv;
        constexpr std::string_view UpgradeCode = "UpgradeCode"sv;
        constexpr std::string_view Markets = "Markets"sv;
        constexpr std::string_view AllowedMarkets = "AllowedMarkets"sv;
        constexpr std::string_view ExcludedMarkets = "ExcludedMarkets"sv;
        constexpr std::string_view ElevationRequirement = "ElevationRequirement"sv;
        constexpr std::string_view ExpectedReturnCodes = "ExpectedReturnCodes"sv;
        constexpr std::string_view InstallerReturnCode = "InstallerReturnCode"sv;
        constexpr std::string_view ReturnResponse = "ReturnResponse"sv;
        constexpr std::string_view ReturnResponseUrl = "ReturnResponseUrl"sv;
        constexpr std::string_view NestedInstallerType = "NestedInstallerType"sv;
        constexpr std::string_view DisplayInstallWarnings = "DisplayInstallWarnings"sv;
        constexpr std::string_view UnsupportedArguments = "UnsupportedArguments"sv;
        constexpr std::string_view NestedInstallerFiles = "NestedInstallerFiles"sv;
        constexpr std::string_view NestedInstallerFileRelativeFilePath = "RelativeFilePath"sv;
        constexpr std::string_view PortableCommandAlias = "PortableCommandAlias"sv;
        constexpr std::string_view InstallationMetadata = "InstallationMetadata"sv;
        constexpr std::string_view DefaultInstallLocation = "DefaultInstallLocation"sv;
        constexpr std::string_view InstallationMetadataFiles = "Files"sv;
        constexpr std::string_view InstallationMetadataRelativeFilePath = "RelativeFilePath"sv;
        constexpr std::string_view FileSha256 = "FileSha256"sv;
        constexpr std::string_view FileType = "FileType"sv;
        constexpr std::string_view InvocationParameter = "InvocationParameter"sv;
        constexpr std::string_view DisplayName = "DisplayName"sv;
        constexpr std::string_view MinimumOSVersion = "MinimumOSVersion"sv;
        constexpr std::string_view DownloadCommandProhibited = "DownloadCommandProhibited"sv;
        constexpr std::string_view RepairBehavior = "RepairBehavior"sv;

        // Installer switches
        constexpr std::string_view InstallerSwitches = "InstallerSwitches"sv;
        constexpr std::string_view Silent = "Silent"sv;
        constexpr std::string_view SilentWithProgress = "SilentWithProgress"sv;
        constexpr std::string_view Interactive = "Interactive"sv;
        constexpr std::string_view InstallLocation = "InstallLocation"sv;
        constexpr std::string_view Log = "Log"sv;
        constexpr std::string_view Upgrade = "Upgrade"sv;
        constexpr std::string_view Custom = "Custom"sv;
        constexpr std::string_view Repair = "Repair"sv;

        constexpr std::string_view InstallerSuccessCodes = "InstallerSuccessCodes"sv;
        constexpr std::string_view UpgradeBehavior = "UpgradeBehavior"sv;
        constexpr std::string_view Commands = "Commands"sv;
        constexpr std::string_view Protocols = "Protocols"sv;
        constexpr std::string_view FileExtensions = "FileExtensions"sv;

        // Dependencies
        constexpr std::string_view Dependencies = "Dependencies"sv;
        constexpr std::string_view WindowsFeatures = "WindowsFeatures"sv;
        constexpr std::string_view WindowsLibraries = "WindowsLibraries"sv;
        constexpr std::string_view PackageDependencies = "PackageDependencies"sv;
        constexpr std::string_view MinimumVersion = "MinimumVersion"sv;
        constexpr std::string_view ExternalDependencies = "ExternalDependencies"sv;
        constexpr std::string_view Capabilities = "Capabilities"sv;
        constexpr std::string_view RestrictedCapabilities = "RestrictedCapabilities"sv;

        // Locale
        constexpr std::string_view Localization = "Localization"sv;
        constexpr std::string_view InstallationNotes = "InstallationNotes"sv;
        constexpr std::string_view PurchaseUrl = "PurchaseUrl"sv;
        constexpr std::string_view Documentations = "Documentations"sv;
        constexpr std::string_view DocumentLabel = "DocumentLabel"sv;
        constexpr std::string_view DocumentUrl = "DocumentUrl"sv;
        constexpr std::string_view Icons = "Icons"sv;
        constexpr std::string_view IconUrl = "IconUrl"sv;
        constexpr std::string_view IconFileType = "IconFileType"sv;
        constexpr std::string_view IconResolution = "IconResolution"sv;
        constexpr std::string_view IconTheme = "IconTheme"sv;
        constexpr std::string_view IconSha256 = "IconSha256"sv;
        constexpr std::string_view ReleaseNotes = "ReleaseNotes"sv;
        constexpr std::string_view ReleaseNotesUrl = "ReleaseNotesUrl"sv;
        constexpr std::string_view Agreements = "Agreements"sv;
        constexpr std::string_view AgreementLabel = "AgreementLabel"sv;
        constexpr std::string_view Agreement = "Agreement"sv;
        constexpr std::string_view AgreementUrl = "AgreementUrl"sv;
        constexpr std::string_view DefaultLocale = "DefaultLocale"sv;
        constexpr std::string_view Locales = "Locales"sv;
        constexpr std::string_view PackageLocale = "PackageLocale"sv;
        constexpr std::string_view Publisher = "Publisher"sv;
        constexpr std::string_view PublisherUrl = "PublisherUrl"sv;
        constexpr std::string_view PublisherSupportUrl = "PublisherSupportUrl"sv;
        constexpr std::string_view PrivacyUrl = "PrivacyUrl"sv;
        constexpr std::string_view Author = "Author"sv;
        constexpr std::string_view PackageName = "PackageName"sv;
        constexpr std::string_view PackageUrl = "PackageUrl"sv;
        constexpr std::string_view License = "License"sv;
        constexpr std::string_view LicenseUrl = "LicenseUrl"sv;
        constexpr std::string_view Copyright = "Copyright"sv;
        constexpr std::string_view CopyrightUrl = "CopyrightUrl"sv;
        constexpr std::string_view ShortDescription = "ShortDescription"sv;
        constexpr std::string_view Description = "Description"sv;
        constexpr std::string_view Tags = "Tags"sv;
        constexpr std::string_view Moniker = "Moniker"sv;

#define WRITE_PROPERTY(emitter, key, value) \
        { \
            emitter << YAML::Key << key << YAML::Value << value; \
        }

#define WRITE_BOOL_PROPERTY(emitter, key, value) \
        { \
            emitter << YAML::Key << key << YAML::Value << Utility::ConvertBoolToString(value); \
        }

#define WRITE_PROPERTY_IF_EXISTS(emitter, key, value) \
        { \
            if (!value.empty()) \
            { \
                WRITE_PROPERTY(emitter, key, value) \
            } \
        }

        void ProcessAgreements(YAML::Emitter& out, const std::vector<AppInstaller::Manifest::Agreement>& agreements)
        {
            if (agreements.empty())
            {
                return;
            }

            out << YAML::Key << Agreements;
            out << YAML::BeginSeq;
            for (const auto& agreement : agreements)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, AgreementLabel, agreement.Label);
                WRITE_PROPERTY_IF_EXISTS(out, AgreementUrl, agreement.AgreementUrl);
                WRITE_PROPERTY_IF_EXISTS(out, Agreement, agreement.AgreementText);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        void ProcessDocumentations(YAML::Emitter& out, const std::vector<AppInstaller::Manifest::Documentation>& documentations)
        {
            if (documentations.empty())
            {
                return;
            }

            out << YAML::Key << Documentations;
            out << YAML::BeginSeq;
            for (const auto& document : documentations)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, DocumentLabel, document.DocumentLabel);
                WRITE_PROPERTY_IF_EXISTS(out, DocumentUrl, document.DocumentUrl);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        void ProcessIcons(YAML::Emitter& out, std::vector<AppInstaller::Manifest::Icon> icons)
        {
            if (icons.empty())
            {
                return;
            }

            out << YAML::Key << Icons;
            out << YAML::BeginSeq;
            for (const auto& icon : icons)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, IconUrl, icon.Url);
                WRITE_PROPERTY_IF_EXISTS(out, IconFileType, IconFileTypeToString(icon.FileType));
                WRITE_PROPERTY_IF_EXISTS(out, IconResolution, IconResolutionToString(icon.Resolution));
                WRITE_PROPERTY_IF_EXISTS(out, IconTheme, IconThemeToString(icon.Theme));
                WRITE_PROPERTY_IF_EXISTS(out, IconSha256, Utility::LocIndString{ Utility::SHA256::ConvertToString(icon.Sha256)});
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        // Generic method for handling a list of strings (i.e. tags)
        void ProcessSequence(YAML::Emitter& out, std::string_view name, std::vector<AppInstaller::Manifest::string_t> items)
        {
            if (items.empty())
            {
                return;
            }

            out << YAML::Key << name;
            out << YAML::BeginSeq;
            for (const auto& item : items)
            {
                out << item;
            }
            out << YAML::EndSeq;
        }

        void ProcessLocaleFields(YAML::Emitter& out, const ManifestLocalization& manifest)
        {
            ProcessAgreements(out, manifest.Get<Localization::Agreements>());
            ProcessDocumentations(out, manifest.Get<Localization::Documentations>());
            ProcessIcons(out, manifest.Get<Localization::Icons>());
            ProcessSequence(out, Tags, manifest.Get<Localization::Tags>());

            WRITE_PROPERTY(out, PackageLocale, manifest.Locale);
            WRITE_PROPERTY_IF_EXISTS(out, Author, manifest.Get<Localization::Author>());
            WRITE_PROPERTY_IF_EXISTS(out, Copyright, manifest.Get<Localization::Copyright>());
            WRITE_PROPERTY_IF_EXISTS(out, CopyrightUrl, manifest.Get<Localization::CopyrightUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, Description, manifest.Get<Localization::Description>());
            WRITE_PROPERTY_IF_EXISTS(out, ShortDescription, manifest.Get<Localization::ShortDescription>());
            WRITE_PROPERTY_IF_EXISTS(out, License, manifest.Get<Localization::License>());
            WRITE_PROPERTY_IF_EXISTS(out, LicenseUrl, manifest.Get<Localization::LicenseUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, PackageName, manifest.Get<Localization::PackageName>());
            WRITE_PROPERTY_IF_EXISTS(out, PackageUrl, manifest.Get<Localization::PackageUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, PrivacyUrl, manifest.Get<Localization::PrivacyUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, Publisher, manifest.Get<Localization::Publisher>());
            WRITE_PROPERTY_IF_EXISTS(out, PublisherSupportUrl, manifest.Get<Localization::PublisherSupportUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, PublisherUrl, manifest.Get<Localization::PublisherUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, PurchaseUrl, manifest.Get<Localization::PurchaseUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, ReleaseNotes, manifest.Get<Localization::ReleaseNotes>());
            WRITE_PROPERTY_IF_EXISTS(out, ReleaseNotesUrl, manifest.Get<Localization::ReleaseNotesUrl>());
            WRITE_PROPERTY_IF_EXISTS(out, InstallationNotes, manifest.Get<Localization::InstallationNotes>());
        }

        void ProcessAppsAndFeaturesEntries(YAML::Emitter& out, const std::vector<AppsAndFeaturesEntry>& appsAndFeaturesEntries)
        {
            if (appsAndFeaturesEntries.empty())
            {
                return;
            }

            out << YAML::Key << AppsAndFeaturesEntries;
            out << YAML::BeginSeq;
            for (const auto& appsAndFeatureEntry : appsAndFeaturesEntries)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, DisplayName, appsAndFeatureEntry.DisplayName);
                WRITE_PROPERTY_IF_EXISTS(out, DisplayVersion, appsAndFeatureEntry.DisplayVersion);

                if (appsAndFeatureEntry.InstallerType != InstallerTypeEnum::Unknown)
                {
                    WRITE_PROPERTY(out, InstallerType, InstallerTypeToString(appsAndFeatureEntry.InstallerType));
                }

                WRITE_PROPERTY_IF_EXISTS(out, ProductCode, appsAndFeatureEntry.ProductCode);
                WRITE_PROPERTY_IF_EXISTS(out, Publisher, appsAndFeatureEntry.Publisher);
                WRITE_PROPERTY_IF_EXISTS(out, UpgradeCode, appsAndFeatureEntry.UpgradeCode);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        void ProcessInstallerSwitches(YAML::Emitter& out, const std::map<InstallerSwitchType, string_t>& installerSwitches)
        {
            if (installerSwitches.empty())
            {
                return;
            }

            out << YAML::Key << InstallerSwitches;
            out << YAML::BeginMap;
            for (auto const& [type, value] : installerSwitches)
            {
                WRITE_PROPERTY_IF_EXISTS(out, InstallerSwitchTypeToString(type), value);
            }
            out << YAML::EndMap;
        }

        void ProcessExpectedReturnCodes(YAML::Emitter& out, const std::map<DWORD, ManifestInstaller::ExpectedReturnCodeInfo>& expectedReturnCodes)
        {
            if (expectedReturnCodes.empty())
            {
                return;
            }

            out << YAML::Key << ExpectedReturnCodes;
            out << YAML::BeginSeq;
            for (const auto& expectedReturnCode : expectedReturnCodes)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, InstallerReturnCode, std::to_string(expectedReturnCode.first));
                WRITE_PROPERTY_IF_EXISTS(out, ReturnResponse, ExpectedReturnCodeToString(expectedReturnCode.second.ReturnResponseEnum));
                WRITE_PROPERTY_IF_EXISTS(out, ReturnResponseUrl, expectedReturnCode.second.ReturnResponseUrl);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        void ProcessUnsupportedArguments(YAML::Emitter& out, const std::vector<UnsupportedArgumentEnum>& unsupportedArguments)
        {
            if (unsupportedArguments.empty())
            {
                return;
            }

            out << YAML::Key << UnsupportedArguments;
            out << YAML::BeginSeq;
            for (auto const& unsupportedArgs : unsupportedArguments)
            {
                out << UnsupportedArgumentToString(unsupportedArgs);
            }
            out << YAML::EndSeq;
        }

        void ProcessUnsupportedOSArchitecture(YAML::Emitter& out, const std::vector<AppInstaller::Utility::Architecture>& architectures)
        {
            if (architectures.empty())
            {
                return;
            }

            out << YAML::Key << UnsupportedOSArchitectures;
            out << YAML::BeginSeq;
            for (auto const& architecture : architectures)
            {
                out << Utility::ToLower(ToString(architecture));
            }
            out << YAML::EndSeq;
        }

        void ProcessInstallModes(YAML::Emitter& out, const std::vector<InstallModeEnum>& installModes)
        {
            if (installModes.empty())
            {
                return;
            }

            out << YAML::Key << InstallModes;
            out << YAML::BeginSeq;
            for (auto const& installMode : installModes)
            {
                out << InstallModeToString(installMode);
            }
            out << YAML::EndSeq;
        }

        void ProcessPlatforms(YAML::Emitter& out, const std::vector<PlatformEnum>& platforms)
        {
            if (platforms.empty())
            {
                return;
            }
            out << YAML::Key << Platform;
            out << YAML::BeginSeq;
            for (auto const& platform : platforms)
            {
                out << PlatformToString(platform);
            }
            out << YAML::EndSeq;
        }

        void ProcessInstallerSuccessCodes(YAML::Emitter& out, const std::vector<DWORD>& installerSuccessCodes)
        {
            if (installerSuccessCodes.empty())
            {
                return;
            }

            out << YAML::Key << InstallerSuccessCodes;
            out << YAML::BeginSeq;
            for (auto const& installerSuccessCode : installerSuccessCodes)
            {
                out << std::to_string(installerSuccessCode);
            }
            out << YAML::EndSeq;
        }

        void ProcessMarkets(YAML::Emitter& out, const MarketsInfo& marketsInfo)
        {
            if (marketsInfo.AllowedMarkets.empty() && marketsInfo.ExcludedMarkets.empty())
            {
                return;
            }

            out << YAML::Key << Markets;
            out << YAML::BeginMap;
            ProcessSequence(out, AllowedMarkets, marketsInfo.AllowedMarkets);
            ProcessSequence(out, ExcludedMarkets, marketsInfo.ExcludedMarkets);
            out << YAML::EndMap;
        }

        void ProcessNestedInstallerFiles(YAML::Emitter& out, const std::vector<NestedInstallerFile>& nestedInstallerFiles)
        {
            if (nestedInstallerFiles.empty())
            {
                return;
            }

            out << YAML::Key << NestedInstallerFiles;
            out << YAML::BeginSeq;
            for (const auto& nestedInstallerFile : nestedInstallerFiles)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, NestedInstallerFileRelativeFilePath, nestedInstallerFile.RelativeFilePath);
                WRITE_PROPERTY_IF_EXISTS(out, PortableCommandAlias, nestedInstallerFile.PortableCommandAlias);
                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        void ProcessInstallationMetadataInstalledFiles(YAML::Emitter& out, const std::vector<InstalledFile>& installedFiles)
        {
            if (installedFiles.empty())
            {
                return;
            }

            out << YAML::Key << InstallationMetadataFiles;
            out << YAML::BeginSeq;
            for (const auto& installedFile : installedFiles)
            {
                out << YAML::BeginMap;
                WRITE_PROPERTY_IF_EXISTS(out, InstallationMetadataRelativeFilePath, installedFile.RelativeFilePath);
                WRITE_PROPERTY_IF_EXISTS(out, FileSha256, Utility::LocIndString{ Utility::SHA256::ConvertToString(installedFile.FileSha256) });
                WRITE_PROPERTY_IF_EXISTS(out, FileType, InstalledFileTypeToString(installedFile.FileType));
                WRITE_PROPERTY_IF_EXISTS(out, InvocationParameter, installedFile.InvocationParameter);
                WRITE_PROPERTY_IF_EXISTS(out, DisplayName, installedFile.DisplayName);

                out << YAML::EndMap;
            }
            out << YAML::EndSeq;
        }

        void ProcessInstallationMetadata(YAML::Emitter& out, const InstallationMetadataInfo& installationMetadata)
        {
            if (installationMetadata.DefaultInstallLocation.empty() && installationMetadata.Files.empty())
            {
                return;
            }

            out << YAML::Key << InstallationMetadata;
            out << YAML::BeginMap;
            WRITE_PROPERTY_IF_EXISTS(out, DefaultInstallLocation, installationMetadata.DefaultInstallLocation);
            ProcessInstallationMetadataInstalledFiles(out, installationMetadata.Files);
            out << YAML::EndMap;
        }

        void ProcessDependencies(YAML::Emitter& out, const DependencyList& dependencies)
        {
            if (!dependencies.HasAny())
            {
                return;
            }

            out << YAML::Key << Dependencies;
            out << YAML::BeginMap;

            if (dependencies.HasAnyOf(DependencyType::WindowsFeature))
            {
                out << YAML::Key << WindowsFeatures;
                out << YAML::BeginSeq;
                dependencies.ApplyToType(DependencyType::WindowsFeature, [&out](Dependency dependency)
                    {
                        out << dependency.Id();
                    });
                out << YAML::EndSeq;
            }

            if (dependencies.HasAnyOf(DependencyType::WindowsLibrary))
            {
                out << YAML::Key << WindowsLibraries;
                out << YAML::BeginSeq;
                dependencies.ApplyToType(DependencyType::WindowsLibrary, [&out](Dependency dependency)
                    {
                        out << dependency.Id();
                    });
                out << YAML::EndSeq;
            }

            if (dependencies.HasAnyOf(DependencyType::Package))
            {
                out << YAML::Key << PackageDependencies;
                out << YAML::BeginSeq;
                dependencies.ApplyToType(DependencyType::Package, [&out](Dependency dependency)
                    {
                        out << YAML::BeginMap;
                        WRITE_PROPERTY_IF_EXISTS(out, PackageIdentifier, dependency.Id());

                        if (dependency.MinVersion.has_value())
                        {
                            WRITE_PROPERTY_IF_EXISTS(out, MinimumVersion, dependency.MinVersion.value().ToString());
                        }

                        out << YAML::EndMap;
                    });
                out << YAML::EndSeq;
            }

            if (dependencies.HasAnyOf(DependencyType::External))
            {
                out << YAML::Key << ExternalDependencies;
                out << YAML::BeginSeq;
                dependencies.ApplyToType(DependencyType::External, [&out](Dependency dependency)
                    {
                        out << dependency.Id();
                    });
                out << YAML::EndSeq;
            }

            out << YAML::EndMap;
        }

        void ProcessInstallerFields(YAML::Emitter& out, const ManifestInstaller& installer)
        {
            WRITE_PROPERTY(out, Architecture, Utility::ToLower(ToString(installer.Arch)));
            WRITE_PROPERTY(out, InstallerType, InstallerTypeToString(installer.BaseInstallerType));

            if (installer.NestedInstallerType != InstallerTypeEnum::Unknown)
            {
                WRITE_PROPERTY(out, NestedInstallerType, InstallerTypeToString(installer.NestedInstallerType));
            }

            if (!installer.Sha256.empty())
            {
                WRITE_PROPERTY(out, InstallerSha256, Utility::SHA256::ConvertToString(installer.Sha256));
            }

            if (!installer.SignatureSha256.empty())
            {
                WRITE_PROPERTY(out, SignatureSha256, Utility::SHA256::ConvertToString(installer.SignatureSha256));
            }
            WRITE_PROPERTY_IF_EXISTS(out, InstallerUrl, installer.Url);
            WRITE_PROPERTY_IF_EXISTS(out, Scope, Utility::ToLower(ScopeToString(installer.Scope)));
            WRITE_PROPERTY_IF_EXISTS(out, InstallerLocale, installer.Locale);

            if (installer.ElevationRequirement != ElevationRequirementEnum::Unknown)
            {
                WRITE_PROPERTY(out, ElevationRequirement, ElevationRequirementToString(installer.ElevationRequirement));
            }

            WRITE_PROPERTY_IF_EXISTS(out, PackageFamilyName, installer.PackageFamilyName);
            WRITE_PROPERTY_IF_EXISTS(out, ReleaseDate, installer.ReleaseDate);
            WRITE_BOOL_PROPERTY(out, InstallerAbortsTerminal, installer.InstallerAbortsTerminal);
            WRITE_BOOL_PROPERTY(out, InstallLocationRequired, installer.InstallLocationRequired);
            WRITE_BOOL_PROPERTY(out, RequireExplicitUpgrade, installer.RequireExplicitUpgrade);
            WRITE_BOOL_PROPERTY(out, DisplayInstallWarnings, installer.DisplayInstallWarnings);
            WRITE_BOOL_PROPERTY(out, DownloadCommandProhibited, installer.DownloadCommandProhibited);
            WRITE_PROPERTY_IF_EXISTS(out, MinimumOSVersion, installer.MinOSVersion);
            WRITE_PROPERTY_IF_EXISTS(out, ProductCode, installer.ProductCode);
            WRITE_PROPERTY_IF_EXISTS(out, UpgradeBehavior, UpdateBehaviorToString(installer.UpdateBehavior));
            WRITE_PROPERTY_IF_EXISTS(out, RepairBehavior, RepairBehaviorToString(installer.RepairBehavior));

            ProcessSequence(out, Capabilities, installer.Capabilities);
            ProcessSequence(out, Commands, installer.Commands);
            ProcessSequence(out, FileExtensions, installer.FileExtensions);
            ProcessSequence(out, Protocols, installer.Protocols);
            ProcessSequence(out, RestrictedCapabilities, installer.RestrictedCapabilities);

            ProcessAppsAndFeaturesEntries(out, installer.AppsAndFeaturesEntries);
            ProcessDependencies(out, installer.Dependencies);
            ProcessExpectedReturnCodes(out, installer.ExpectedReturnCodes);
            ProcessInstallerSwitches(out, installer.Switches);
            ProcessInstallationMetadata(out, installer.InstallationMetadata);
            ProcessInstallerSuccessCodes(out, installer.InstallerSuccessCodes);
            ProcessInstallModes(out, installer.InstallModes);
            ProcessMarkets(out, installer.Markets);
            ProcessNestedInstallerFiles(out, installer.NestedInstallerFiles);
            ProcessPlatforms(out, installer.Platform);
            ProcessUnsupportedArguments(out, installer.UnsupportedArguments);
            ProcessUnsupportedOSArchitecture(out, installer.UnsupportedOSArchitectures);
        }

        void ProcessInstaller(YAML::Emitter& out, const ManifestInstaller& installer)
        {
            out << YAML::Key << Installers;
            out << YAML::BeginSeq;
            out << YAML::BeginMap;
            ProcessInstallerFields(out, installer);
            out << YAML::EndMap;
            out << YAML::EndSeq;
        }

        void ProcessLocalizations(YAML::Emitter& out, const std::vector<ManifestLocalization>& localizations)
        {
            if (!localizations.empty())
            {
                out << YAML::Key << Localization;
                out << YAML::BeginSeq;
                out << YAML::BeginMap;

                for (const auto& localization : localizations)
                {
                    ProcessLocaleFields(out, localization);
                }

                out << YAML::EndMap;
                out << YAML::EndSeq;
            }
        }

        void PopulateManifestYamlEmitter(YAML::Emitter& out, const Manifest& manifest, const ManifestInstaller& installer)
        {
            // Currently, exporting the yaml only supports outputting a single installer.
            // TODO: If no single installer is provided, output all installers.
            out << YAML::BeginMap;
            WRITE_PROPERTY(out, PackageIdentifier, manifest.Id);
            WRITE_PROPERTY(out, PackageVersion, manifest.Version);
            WRITE_PROPERTY_IF_EXISTS(out, Channel, manifest.Channel);
            WRITE_PROPERTY_IF_EXISTS(out, Moniker, manifest.Moniker);
            ProcessLocaleFields(out, manifest.DefaultLocalization);
            ProcessLocalizations(out, manifest.Localizations);
            ProcessInstaller(out, installer);
            WRITE_PROPERTY(out, ManifestVersion, manifest.ManifestVersion.ToString());

            ManifestTypeEnum manifestType = ManifestTypeEnum::Merged;
            WRITE_PROPERTY(out, ManifestType, ManifestTypeToString(manifestType));
            out << YAML::EndMap;
        }
    }

    std::string YamlWriter::ManifestToYamlString(const Manifest& manifest, const ManifestInstaller& installer)
    {
        YAML::Emitter out;
        PopulateManifestYamlEmitter(out, manifest, installer);
        return out.str();
    }

    void YamlWriter::OutputYamlFile(const Manifest& manifest, const ManifestInstaller& installer, const std::filesystem::path& out)
    {
        const std::filesystem::path& parentDirectory = out.parent_path();

        if (!std::filesystem::exists(parentDirectory))
        {
            std::filesystem::create_directories(parentDirectory);
        }
        else
        {
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE), !std::filesystem::is_directory(parentDirectory));
        }

        YAML::Emitter emitter;
        PopulateManifestYamlEmitter(emitter, manifest, installer);

        std::ofstream outFileStream(out);
        emitter.Emit(outFileStream);
        outFileStream.close();
    }
}