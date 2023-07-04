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

#define ADD_PROPERTY_NODE(emitter, key, value) \
        { \
            if (!value.empty()) \
            { \
                emitter << YAML::Key << key << YAML::Value << value; \
            } \
        }
    }

    void ProcessDocumentations(YAML::Emitter& out, std::vector<AppInstaller::Manifest::Documentation> documentations)
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
            ADD_PROPERTY_NODE(out, DocumentLabel, document.DocumentLabel);
            ADD_PROPERTY_NODE(out, DocumentUrl, document.DocumentUrl);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    void ProcessAgreements(YAML::Emitter& out, std::vector<AppInstaller::Manifest::Agreement> agreements)
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
            ADD_PROPERTY_NODE(out, AgreementLabel, agreement.Label);
            ADD_PROPERTY_NODE(out, AgreementUrl, agreement.AgreementUrl);
            ADD_PROPERTY_NODE(out, Agreement, agreement.AgreementText);
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
            ADD_PROPERTY_NODE(out, IconUrl, icon.Url);
            ADD_PROPERTY_NODE(out, IconFileType, IconFileTypeToString(icon.FileType));
            ADD_PROPERTY_NODE(out, IconResolution, IconResolutionToString(icon.Resolution));
            ADD_PROPERTY_NODE(out, IconTheme, IconThemeToString(icon.Theme));
            ADD_PROPERTY_NODE(out, IconSha256, Utility::LocIndString{ Utility::SHA256::ConvertToString(icon.Sha256)});
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    // Generic method for handling a list of strings (i.e. tags)
    void ProcessSequence(YAML::Emitter& out, AppInstaller::Utility::LocIndView name, std::vector<AppInstaller::Manifest::string_t> items)
    {
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

        ADD_PROPERTY_NODE(out, Author, manifest.Get<Localization::Author>());
        ADD_PROPERTY_NODE(out, Copyright, manifest.Get<Localization::Copyright>());
        ADD_PROPERTY_NODE(out, CopyrightUrl, manifest.Get<Localization::CopyrightUrl>());
        ADD_PROPERTY_NODE(out, Description, manifest.Get<Localization::Description>());
        ADD_PROPERTY_NODE(out, ShortDescription, manifest.Get<Localization::ShortDescription>());
        ADD_PROPERTY_NODE(out, License, manifest.Get<Localization::License>());
        ADD_PROPERTY_NODE(out, LicenseUrl, manifest.Get<Localization::LicenseUrl>());
        ADD_PROPERTY_NODE(out, PackageName, manifest.Get<Localization::PackageName>());
        ADD_PROPERTY_NODE(out, PackageUrl, manifest.Get<Localization::PackageUrl>());
        ADD_PROPERTY_NODE(out, PrivacyUrl, manifest.Get<Localization::PrivacyUrl>());
        ADD_PROPERTY_NODE(out, Publisher, manifest.Get<Localization::Publisher>());
        ADD_PROPERTY_NODE(out, PublisherSupportUrl, manifest.Get<Localization::PublisherSupportUrl>());
        ADD_PROPERTY_NODE(out, PublisherUrl, manifest.Get<Localization::PublisherUrl>());
        ADD_PROPERTY_NODE(out, PurchaseUrl, manifest.Get<Localization::PurchaseUrl>());
        ADD_PROPERTY_NODE(out, ReleaseNotes, manifest.Get<Localization::ReleaseNotes>());
        ADD_PROPERTY_NODE(out, ReleaseNotesUrl, manifest.Get<Localization::ReleaseNotesUrl>());
        ADD_PROPERTY_NODE(out, InstallationNotes, manifest.Get<Localization::InstallationNotes>());
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
            ADD_PROPERTY_NODE(out, DisplayName, appsAndFeatureEntry.DisplayName);
            ADD_PROPERTY_NODE(out, DisplayVersion, appsAndFeatureEntry.DisplayVersion);
            ADD_PROPERTY_NODE(out, InstallerType, InstallerTypeToString(appsAndFeatureEntry.InstallerType));
            ADD_PROPERTY_NODE(out, ProductCode, appsAndFeatureEntry.ProductCode);
            ADD_PROPERTY_NODE(out, Publisher, appsAndFeatureEntry.Publisher);
            ADD_PROPERTY_NODE(out, UpgradeCode, appsAndFeatureEntry.UpgradeCode);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    void ProcessInstallerSwitches(YAML::Emitter& out, const std::map<InstallerSwitchType, string_t>& installerSwitches)
    {
        out << YAML::Key << InstallerSwitches;
        out << YAML::BeginMap;
        for (auto const& [type, value] : installerSwitches)
        {
            ADD_PROPERTY_NODE(out, InstallerSwitchTypeToString(type), value);
        }
        out << YAML::EndMap;
    }

    void ProcessExpectedReturnCodes(YAML::Emitter& out, const std::map<DWORD, ManifestInstaller::ExpectedReturnCodeInfo>& expectedReturnCodes)
    {
        out << YAML::Key << ExpectedReturnCodes;
        out << YAML::BeginSeq;
        for (auto const& [installerReturnCode, expectedReturnCodeInfo] : expectedReturnCodes)
        {
            out << YAML::BeginMap;
            ADD_PROPERTY_NODE(out, InstallerReturnCode, std::to_string(installerReturnCode));
            ADD_PROPERTY_NODE(out, ReturnResponse, ExpectedReturnCodeToString(expectedReturnCodeInfo.ReturnResponseEnum));
            ADD_PROPERTY_NODE(out, ReturnResponseUrl, expectedReturnCodeInfo.ReturnResponseUrl);
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    void ProcessInstallers(YAML::Emitter& out, const std::vector<ManifestInstaller>& installers)
    {
        out << YAML::Key << Installers;
        out << YAML::BeginSeq;
        for (const auto& installer : installers)
        {
            out << YAML::BeginMap;
            ProcessAppsAndFeaturesEntries(out, installer.AppsAndFeaturesEntries);
            ADD_PROPERTY_NODE(out, Architecture, ToString(installer.Arch));
            ADD_PROPERTY_NODE(out, InstallerLocale, installer.Locale);
            ADD_PROPERTY_NODE(out, InstallerType, InstallerTypeToString(installer.BaseInstallerType));
            ADD_PROPERTY_NODE(out, NestedInstallerType, InstallerTypeToString(installer.NestedInstallerType));
            ProcessSequence(out, Capabilities, installer.Capabilities);
            ProcessSequence(out, Commands, installer.Commands);
            ProcessSequence(out, FileExtensions, installer.FileExtensions);
            ProcessSequence(out, Protocols, installer.Protocols);
            ProcessSequence(out, RestrictedCapabilities, installer.RestrictedCapabilities);
            ADD_PROPERTY_NODE(out, ElevationRequirement, ElevationRequirementToString(installer.ElevationRequirement));
            ADD_PROPERTY_NODE(out, Scope, ScopeToString(installer.Scope));
            ADD_PROPERTY_NODE(out, InstallerSha256, Utility::SHA256::ConvertToString(installer.Sha256));
            ADD_PROPERTY_NODE(out, InstallerSha256, Utility::SHA256::ConvertToString(installer.SignatureSha256));
            ADD_PROPERTY_NODE(out, PackageFamilyName, installer.PackageFamilyName);
            ADD_PROPERTY_NODE(out, ReleaseDate, installer.ReleaseDate);
            ADD_PROPERTY_NODE(out, InstallerAbortsTerminal, BoolToString(installer.InstallerAbortsTerminal));
            ADD_PROPERTY_NODE(out, InstallLocationRequired, BoolToString(installer.InstallLocationRequired));
            ADD_PROPERTY_NODE(out, RequireExplicitUpgrade, BoolToString(installer.RequireExplicitUpgrade));
            ADD_PROPERTY_NODE(out, DisplayInstallWarnings, BoolToString(installer.DisplayInstallWarnings));

            // ProcessSequence(out, UnsupportedArguments, installer.UnsupportedArguments);
            // ProcessSequence(out, UnsupportedOSArchitectures, installer.UnsupportedOSArchitectures);
            ProcessInstallerSwitches(out, installer.Switches);
            ProcessExpectedReturnCodes(out, installer.ExpectedReturnCodes);

            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    std::string ManifestYamlDepopulator::DepopulateManifest(const Manifest& manifest)
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        // Add all basic properties.
        ADD_PROPERTY_NODE(out, PackageIdentifier, manifest.Id);
        ADD_PROPERTY_NODE(out, PackageVersion, manifest.Version);
        ADD_PROPERTY_NODE(out, Channel, manifest.Channel);
        ADD_PROPERTY_NODE(out, Moniker, manifest.Moniker);
        ADD_PROPERTY_NODE(out, ManifestVersion, manifest.ManifestVersion.ToString());
        ProcessInstallers(out, manifest.Installers);
        ProcessLocaleFields(out, manifest.DefaultLocalization);

        out << YAML::EndMap;

        return out.str();
    }

}