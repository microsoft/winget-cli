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
        constexpr Utility::LocIndView PackageIdentifier = "PackageIdentifier"_liv;
        constexpr Utility::LocIndView PackageFamilyName = "PackageFamilyName"_liv;
        constexpr Utility::LocIndView ProductCode = "ProductCode"_liv;
        constexpr Utility::LocIndView Versions = "Versions"_liv;
        constexpr Utility::LocIndView PackageVersion = "PackageVersion"_liv;
        constexpr Utility::LocIndView Channel = "Channel"_liv;
        constexpr Utility::LocIndView ManifestVersion = "ManifestVersion"_liv;
        constexpr Utility::LocIndView ManifestType = "ManifestType"_liv;

        // Installer
        constexpr Utility::LocIndView Installers = "Installers"_liv;
        constexpr Utility::LocIndView InstallerIdentifier = "InstallerIdentifier"_liv;
        constexpr Utility::LocIndView InstallerSha256 = "InstallerSha256"_liv;
        constexpr Utility::LocIndView InstallerUrl = "InstallerUrl"_liv;
        constexpr Utility::LocIndView Architecture = "Architecture"_liv;
        constexpr Utility::LocIndView InstallerLocale = "InstallerLocale"_liv;
        constexpr Utility::LocIndView Platform = "Platform"_liv;
        constexpr Utility::LocIndView MinimumO_liversion = "MinimumO_liversion"_liv;
        constexpr Utility::LocIndView InstallerType = "InstallerType"_liv;
        constexpr Utility::LocIndView Scope = "Scope"_liv;
        constexpr Utility::LocIndView SignatureSha256 = "SignatureSha256"_liv;
        constexpr Utility::LocIndView InstallModes = "InstallModes"_liv;
        constexpr Utility::LocIndView MSStoreProductIdentifier = "MSStoreProductIdentifier"_liv;
        constexpr Utility::LocIndView ReleaseDate = "ReleaseDate"_liv;
        constexpr Utility::LocIndView InstallerAbortsTerminal = "InstallerAbortsTerminal"_liv;
        constexpr Utility::LocIndView InstallLocationRequired = "InstallLocationRequired"_liv;
        constexpr Utility::LocIndView RequireExplicitUpgrade = "RequireExplicitUpgrade"_liv;
        constexpr Utility::LocIndView UnsupportedOSArchitectures = "UnsupportedOSArchitectures"_liv;
        constexpr Utility::LocIndView AppsAndFeaturesEntries = "AppsAndFeaturesEntries"_liv;
        constexpr Utility::LocIndView DisplayVersion = "DisplayVersion"_liv;
        constexpr Utility::LocIndView UpgradeCode = "UpgradeCode"_liv;
        constexpr Utility::LocIndView Markets = "Markets"_liv;
        constexpr Utility::LocIndView AllowedMarkets = "AllowedMarkets"_liv;
        constexpr Utility::LocIndView ExcludedMarkets = "ExcludedMarkets"_liv;
        constexpr Utility::LocIndView ElevationRequirement = "ElevationRequirement"_liv;
        constexpr Utility::LocIndView ExpectedReturnCodes = "ExpectedReturnCodes"_liv;
        constexpr Utility::LocIndView InstallerReturnCode = "InstallerReturnCode"_liv;
        constexpr Utility::LocIndView ReturnResponse = "ReturnResponse"_liv;
        constexpr Utility::LocIndView ReturnResponseUrl = "ReturnResponseUrl"_liv;
        constexpr Utility::LocIndView NestedInstallerType = "NestedInstallerType"_liv;
        constexpr Utility::LocIndView DisplayInstallWarnings = "DisplayInstallWarnings"_liv;
        constexpr Utility::LocIndView UnsupportedArguments = "UnsupportedArguments"_liv;
        constexpr Utility::LocIndView NestedInstallerFiles = "NestedInstallerFiles"_liv;
        constexpr Utility::LocIndView NestedInstallerFileRelativeFilePath = "RelativeFilePath"_liv;
        constexpr Utility::LocIndView PortableCommandAlias = "PortableCommandAlias"_liv;
        constexpr Utility::LocIndView InstallationMetadata = "InstallationMetadata"_liv;
        constexpr Utility::LocIndView DefaultInstallLocation = "DefaultInstallLocation"_liv;
        constexpr Utility::LocIndView InstallationMetadataFiles = "Files"_liv;
        constexpr Utility::LocIndView InstallationMetadataRelativeFilePath = "RelativeFilePath"_liv;
        constexpr Utility::LocIndView FileSha256 = "FileSha256"_liv;
        constexpr Utility::LocIndView FileType = "FileType"_liv;
        constexpr Utility::LocIndView InvocationParameter = "InvocationParameter"_liv;
        constexpr Utility::LocIndView DisplayName = "DisplayName"_liv;
        constexpr Utility::LocIndView MinimumOSVersion = "MinimumOSVersion"_liv;

        // Installer switches
        constexpr Utility::LocIndView InstallerSwitches = "InstallerSwitches"_liv;
        constexpr Utility::LocIndView Silent = "Silent"_liv;
        constexpr Utility::LocIndView SilentWithProgress = "SilentWithProgress"_liv;
        constexpr Utility::LocIndView Interactive = "Interactive"_liv;
        constexpr Utility::LocIndView InstallLocation = "InstallLocation"_liv;
        constexpr Utility::LocIndView Log = "Log"_liv;
        constexpr Utility::LocIndView Upgrade = "Upgrade"_liv;
        constexpr Utility::LocIndView Custom = "Custom"_liv;

        constexpr Utility::LocIndView InstallerSuccessCodes = "InstallerSuccessCodes"_liv;
        constexpr Utility::LocIndView UpgradeBehavior = "UpgradeBehavior"_liv;
        constexpr Utility::LocIndView Commands = "Commands"_liv;
        constexpr Utility::LocIndView Protocols = "Protocols"_liv;
        constexpr Utility::LocIndView FileExtensions = "FileExtensions"_liv;

        // Dependencies
        constexpr Utility::LocIndView Dependencies = "Dependencies"_liv;
        constexpr Utility::LocIndView WindowsFeatures = "WindowsFeatures"_liv;
        constexpr Utility::LocIndView WindowsLibraries = "WindowsLibraries"_liv;
        constexpr Utility::LocIndView PackageDependencies = "PackageDependencies"_liv;
        constexpr Utility::LocIndView MinimumVersion = "MinimumVersion"_liv;
        constexpr Utility::LocIndView ExternalDependencies = "ExternalDependencies"_liv;
        constexpr Utility::LocIndView Capabilities = "Capabilities"_liv;
        constexpr Utility::LocIndView RestrictedCapabilities = "RestrictedCapabilities"_liv;

        // Locale
        constexpr Utility::LocIndView InstallationNotes = "InstallationNotes"_liv;
        constexpr Utility::LocIndView PurchaseUrl = "PurchaseUrl"_liv;
        constexpr Utility::LocIndView Documentations = "Documentations"_liv;
        constexpr Utility::LocIndView DocumentLabel = "DocumentLabel"_liv;
        constexpr Utility::LocIndView DocumentUrl = "DocumentUrl"_liv;
        constexpr Utility::LocIndView Icons = "Icons"_liv;
        constexpr Utility::LocIndView IconUrl = "IconUrl"_liv;
        constexpr Utility::LocIndView IconFileType = "IconFileType"_liv;
        constexpr Utility::LocIndView IconResolution = "IconResolution"_liv;
        constexpr Utility::LocIndView IconTheme = "IconTheme"_liv;
        constexpr Utility::LocIndView IconSha256 = "IconSha256"_liv;
        constexpr Utility::LocIndView ReleaseNotes = "ReleaseNotes"_liv;
        constexpr Utility::LocIndView ReleaseNotesUrl = "ReleaseNotesUrl"_liv;
        constexpr Utility::LocIndView Agreements = "Agreements"_liv;
        constexpr Utility::LocIndView AgreementLabel = "AgreementLabel"_liv;
        constexpr Utility::LocIndView Agreement = "Agreement"_liv;
        constexpr Utility::LocIndView AgreementUrl = "AgreementUrl"_liv;
        constexpr Utility::LocIndView DefaultLocale = "DefaultLocale"_liv;
        constexpr Utility::LocIndView Locales = "Locales"_liv;
        constexpr Utility::LocIndView PackageLocale = "PackageLocale"_liv;
        constexpr Utility::LocIndView Publisher = "Publisher"_liv;
        constexpr Utility::LocIndView PublisherUrl = "PublisherUrl"_liv;
        constexpr Utility::LocIndView PublisherSupportUrl = "PublisherSupportUrl"_liv;
        constexpr Utility::LocIndView PrivacyUrl = "PrivacyUrl"_liv;
        constexpr Utility::LocIndView Author = "Author"_liv;
        constexpr Utility::LocIndView PackageName = "PackageName"_liv;
        constexpr Utility::LocIndView PackageUrl = "PackageUrl"_liv;
        constexpr Utility::LocIndView License = "License"_liv;
        constexpr Utility::LocIndView LicenseUrl = "LicenseUrl"_liv;
        constexpr Utility::LocIndView Copyright = "Copyright"_liv;
        constexpr Utility::LocIndView CopyrightUrl = "CopyrightUrl"_liv;
        constexpr Utility::LocIndView ShortDescription = "ShortDescription"_liv;
        constexpr Utility::LocIndView Description = "Description"_liv;
        constexpr Utility::LocIndView Tags = "Tags"_liv;
        constexpr Utility::LocIndView Moniker = "Moniker"_liv;

#define WRITE_PROPERTY_IF_EXISTS(emitter, key, value) \
        { \
            if (!value.empty()) \
            { \
                emitter << YAML::Key << key << YAML::Value << value; \
            } \
        }
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
    void ProcessSequence(YAML::Emitter& out, AppInstaller::Utility::LocIndView name, std::vector<AppInstaller::Manifest::string_t> items)
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

        WRITE_PROPERTY_IF_EXISTS(out, PackageLocale, manifest.Locale);
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
            WRITE_PROPERTY_IF_EXISTS(out, InstallerType, InstallerTypeToString(appsAndFeatureEntry.InstallerType));
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
            out << ToString(architecture);
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
                    WRITE_PROPERTY_IF_EXISTS(out, MinimumVersion, dependency.MinVersion.value().ToString());
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

    void ProcessInstallers(YAML::Emitter& out, const std::vector<ManifestInstaller>& installers)
    {
        out << YAML::Key << Installers;
        out << YAML::BeginSeq;
        for (const auto& installer : installers)
        {
            out << YAML::BeginMap;
            WRITE_PROPERTY_IF_EXISTS(out, Architecture, ToString(installer.Arch));
            WRITE_PROPERTY_IF_EXISTS(out, InstallerType, InstallerTypeToString(installer.BaseInstallerType));

            if (installer.NestedInstallerType != InstallerTypeEnum::Unknown)
            {
                WRITE_PROPERTY_IF_EXISTS(out, NestedInstallerType, InstallerTypeToString(installer.NestedInstallerType));
            }

            WRITE_PROPERTY_IF_EXISTS(out, InstallerSha256, Utility::SHA256::ConvertToString(installer.Sha256));
            WRITE_PROPERTY_IF_EXISTS(out, SignatureSha256, Utility::SHA256::ConvertToString(installer.SignatureSha256));
            WRITE_PROPERTY_IF_EXISTS(out, InstallerUrl, installer.Url);
            WRITE_PROPERTY_IF_EXISTS(out, Scope, ScopeToString(installer.Scope));
            WRITE_PROPERTY_IF_EXISTS(out, InstallerLocale, installer.Locale);
            WRITE_PROPERTY_IF_EXISTS(out, ElevationRequirement, ElevationRequirementToString(installer.ElevationRequirement));
            WRITE_PROPERTY_IF_EXISTS(out, PackageFamilyName, installer.PackageFamilyName);
            WRITE_PROPERTY_IF_EXISTS(out, ReleaseDate, installer.ReleaseDate);
            WRITE_PROPERTY_IF_EXISTS(out, InstallerAbortsTerminal, BoolToString(installer.InstallerAbortsTerminal));
            WRITE_PROPERTY_IF_EXISTS(out, InstallLocationRequired, BoolToString(installer.InstallLocationRequired));
            WRITE_PROPERTY_IF_EXISTS(out, RequireExplicitUpgrade, BoolToString(installer.RequireExplicitUpgrade));
            WRITE_PROPERTY_IF_EXISTS(out, DisplayInstallWarnings, BoolToString(installer.DisplayInstallWarnings));
            WRITE_PROPERTY_IF_EXISTS(out, MinimumOSVersion, installer.MinOSVersion);
            WRITE_PROPERTY_IF_EXISTS(out, ProductCode, installer.ProductCode);
            WRITE_PROPERTY_IF_EXISTS(out, UpgradeBehavior, UpdateBehaviorToString(installer.UpdateBehavior));

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
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    std::string ManifestYamlDepopulator::DepopulateManifest(const Manifest& manifest)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        WRITE_PROPERTY_IF_EXISTS(out, PackageIdentifier, manifest.Id);
        WRITE_PROPERTY_IF_EXISTS(out, PackageVersion, manifest.Version);
        WRITE_PROPERTY_IF_EXISTS(out, Channel, manifest.Channel);
        WRITE_PROPERTY_IF_EXISTS(out, Moniker, manifest.Moniker);
        ProcessLocaleFields(out, manifest.DefaultLocalization);
        ProcessInstallers(out, manifest.Installers);
        WRITE_PROPERTY_IF_EXISTS(out, ManifestVersion, manifest.ManifestVersion.ToString());
        WRITE_PROPERTY_IF_EXISTS(out, ManifestType, "singleton"sv);
        out << YAML::EndMap;
        return out.str();
    }

    void OutputYamlDoc(const Manifest& manifest, const std::filesystem::path& out)
    {
        const auto& manifestYamlOutput = ManifestYamlDepopulator::DepopulateManifest(manifest);
        std::filesystem::create_directories(out.parent_path());
        std::ofstream outFileStream(out);
        outFileStream << manifestYamlOutput;
        outFileStream.close();
    }
}