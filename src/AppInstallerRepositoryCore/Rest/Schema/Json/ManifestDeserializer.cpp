// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/HttpClientHelper.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "ManifestDeserializer.h"
#include "JsonHelper.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::Json
{
    namespace
    {
        // Manifest response constants specific to this deserializer
        constexpr std::string_view PackageIdentifier = "PackageIdentifier"sv;
        constexpr std::string_view PackageFamilyName = "PackageFamilyName"sv;
        constexpr std::string_view ProductCode = "ProductCode"sv;
        constexpr std::string_view Versions = "Versions"sv;
        constexpr std::string_view PackageVersion = "PackageVersion"sv;
        constexpr std::string_view Channel = "Channel"sv;

        // Locale
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

        // Installer
        constexpr std::string_view Installers = "Installers"sv;
        constexpr std::string_view InstallerIdentifier = "InstallerIdentifier"sv;
        constexpr std::string_view InstallerSha256 = "InstallerSha256"sv;
        constexpr std::string_view InstallerUrl = "InstallerUrl"sv;
        constexpr std::string_view Architecture = "Architecture"sv;
        constexpr std::string_view InstallerLocale = "InstallerLocale"sv;
        constexpr std::string_view Platform = "Platform"sv;
        constexpr std::string_view MinimumOSVersion = "MinimumOSVersion"sv;
        constexpr std::string_view InstallerType = "InstallerType"sv;
        constexpr std::string_view Scope = "Scope"sv;
        constexpr std::string_view SignatureSha256 = "SignatureSha256"sv;
        constexpr std::string_view InstallModes = "InstallModes"sv;

        // Installer switches
        constexpr std::string_view InstallerSwitches = "InstallerSwitches"sv;
        constexpr std::string_view Silent = "Silent"sv;
        constexpr std::string_view SilentWithProgress = "SilentWithProgress"sv;
        constexpr std::string_view Interactive = "Interactive"sv;
        constexpr std::string_view InstallLocation = "InstallLocation"sv;
        constexpr std::string_view Log = "Log"sv;
        constexpr std::string_view Upgrade = "Upgrade"sv;
        constexpr std::string_view Custom = "Custom"sv;

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
    }

    std::optional<Manifest::Manifest> ManifestDeserializer::Deserialize(const web::json::value& manifestItem) const
    {
        // Make manifest from json output.
        std::optional<Manifest::Manifest> manifest;
        try
        {
            manifest = DeserializeVersion(manifestItem);
        }
        catch (...)
        {
            AICLI_LOG(Repo, Verbose, << "Error encountered while deserializing manifest...");
        }

        return manifest;
    }

    std::optional<Manifest::Manifest> ManifestDeserializer::DeserializeVersion(const web::json::value& manifestJsonObject) const
    {
        if (manifestJsonObject.is_null())
        {
            return {};
        }

        Manifest::Manifest manifest;
        std::optional<std::string> id = JsonHelper::GetRawStringValueFromJsonNode(manifestJsonObject, JsonHelper::GetJsonKeyNameString(PackageIdentifier));
        if (!JsonHelper::IsValidNonEmptyStringValue(id))
        {
            AICLI_LOG (Repo, Verbose, << "Missing package identifier.");
            return {};
        }
        manifest.Id = std::move(id.value());
        
        std::optional<std::reference_wrapper<const web::json::array>> versions = JsonHelper::GetRawJsonArrayFromJsonNode(manifestJsonObject, JsonHelper::GetJsonKeyNameString(Versions));
        if (!versions.has_value() || versions.value().get().size() == 0)
        {
            AICLI_LOG(Repo, Verbose, << "Missing versions in package: " << manifest.Id);
            return {};
        }

        // TODO: Handle multiple versions. Take first?
        auto& versionItem = versions.value().get().at(0);
        std::optional<std::string> packageVersion = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(PackageVersion));
        if (!JsonHelper::IsValidNonEmptyStringValue(packageVersion))
        {
            AICLI_LOG(Repo, Verbose, << "Missing package version in package: " << manifest.Id);
            return {};
        }
        manifest.Version = std::move(packageVersion.value());

        manifest.Channel = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(Channel)).value_or("");

        // Default locale
        auto& defaultLocale = versionItem.at(JsonHelper::GetJsonKeyNameString(DefaultLocale));
        std::optional<Manifest::ManifestLocalization> defaultLocaleObject = DeserializeLocale(defaultLocale);
        if (!defaultLocaleObject.has_value())
        {
            AICLI_LOG(Repo, Verbose, << "Missing default locale in package: " << manifest.Id);
            return {};
        }
        manifest.DefaultLocalization = std::move(defaultLocaleObject.value());

        // Moniker is in Default locale
        manifest.Moniker = JsonHelper::GetRawStringValueFromJsonNode(defaultLocale, JsonHelper::GetJsonKeyNameString(Moniker)).value_or("");
        
        // Installers
        std::optional<std::reference_wrapper<const web::json::array>> installers = JsonHelper::GetRawJsonArrayFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(Installers));
        if (!installers.has_value() || installers.value().get().size() == 0)
        {
            AICLI_LOG(Repo, Verbose, << "Missing installers in package: " << manifest.Id);
            return {};
        }

        for (auto& installer : installers.value().get())
        {
            std::optional<Manifest::ManifestInstaller> installerObject = DeserializeInstaller(installer);
            if (installerObject.has_value())
            {
                manifest.Installers.emplace_back(std::move(installerObject.value()));
            }
        }

        // Other locales
        std::optional<std::reference_wrapper<const web::json::array>> locales = JsonHelper::GetRawJsonArrayFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(Locales));
        if (locales.has_value())
        {
            for (auto& locale : locales.value().get())
            {
                std::optional<Manifest::ManifestLocalization> localeObject = DeserializeLocale(locale);
                if (localeObject.has_value())
                {
                    manifest.Localizations.emplace_back(std::move(localeObject.value()));
                }
            }
        }

        return manifest;
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        if (localeJsonObject.is_null())
        {
            return {};
        }

        Manifest::ManifestLocalization locale;
        std::optional<std::string> packageLocale = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(PackageLocale));
        if (!JsonHelper::IsValidNonEmptyStringValue(packageLocale))
        {
            AICLI_LOG(Repo, Verbose, << "Missing package locale.");
            return {};
        }
        locale.Locale = std::move(packageLocale.value());

        std::optional<std::string> packageName = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(PackageName));
        if (!JsonHelper::IsValidNonEmptyStringValue(packageName))
        {
            AICLI_LOG(Repo, Verbose, << "Missing package name.");
            return {};
        }
        locale.Add<AppInstaller::Manifest::Localization::PackageName>(std::move(packageName.value()));

        std::optional<std::string> publisher = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(Publisher));
        if (!JsonHelper::IsValidNonEmptyStringValue(publisher))
        {
            AICLI_LOG(Repo, Verbose, << "Missing publisher.");
            return {};
        }
        locale.Add<AppInstaller::Manifest::Localization::Publisher>(std::move(publisher.value()));

        std::optional<std::string> shortDescription = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(ShortDescription));
        if (!JsonHelper::IsValidNonEmptyStringValue(shortDescription))
        {
            AICLI_LOG(Repo, Verbose, << "Missing short description.");
            return {};
        }
        locale.Add<AppInstaller::Manifest::Localization::ShortDescription>(std::move(shortDescription.value()));

        locale.Add<AppInstaller::Manifest::Localization::PublisherSupportUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(PublisherSupportUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::PrivacyUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(PrivacyUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Author>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(Author)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::PackageUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(PackageUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::License>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(License)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::LicenseUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(LicenseUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Copyright>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(Copyright)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::CopyrightUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(CopyrightUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Description>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(Description)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Tags>(JsonHelper::GetRawStringArrayFromJsonNode(localeJsonObject, JsonHelper::GetJsonKeyNameString(Tags)));

        return locale;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        if (installerJsonObject.is_null())
        {
            return {};
        }

        Manifest::ManifestInstaller installer;
        std::optional<std::string> url = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(InstallerUrl));
        if (!JsonHelper::IsValidNonEmptyStringValue(url))
        {
            AICLI_LOG(Repo, Verbose, << "Missing installer url.");
            return {};
        }
        installer.Url = std::move(url.value());

        std::optional<std::string> sha256 = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(InstallerSha256));
        if (!JsonHelper::IsValidNonEmptyStringValue(sha256))
        {
            AICLI_LOG(Repo, Verbose, << "Missing installer SHA256.");
            return {};
        }
        installer.Sha256 = Utility::SHA256::ConvertToBytes(sha256.value());

        std::optional<std::string> arch = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(Architecture));
        if (!JsonHelper::IsValidNonEmptyStringValue(arch))
        {
            AICLI_LOG(Repo, Verbose, << "Missing installer architecture.");
            return {};
        }
        installer.Arch = Utility::ConvertToArchitectureEnum(arch.value());

        installer.InstallerType = Manifest::ConvertToInstallerTypeEnum(
            JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(InstallerType)).value_or(""));
        installer.Locale = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(InstallerLocale)).value_or("");

        // platform
        std::optional<std::reference_wrapper<const web::json::array>> platforms = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(Platform));
        if (platforms.has_value())
        {
            for (auto& platform : platforms.value().get())
            {
                std::optional<std::string> platformValue = JsonHelper::GetRawStringValueFromJsonValue(platform);
                if (platformValue.has_value())
                {
                    installer.Platform.emplace_back(Manifest::ConvertToPlatformEnum(platformValue.value()));
                }
            }
        }

        installer.MinOSVersion = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(MinimumOSVersion)).value_or("");
        std::optional<std::string> scope = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(Scope));
        if (scope.has_value())
        {
            installer.Scope = Manifest::ConvertToScopeEnum(scope.value());
        }

        std::optional<std::string> signatureSha256 = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(SignatureSha256));
        if (signatureSha256.has_value())
        {
            installer.SignatureSha256 = Utility::SHA256::ConvertToBytes(signatureSha256.value());
        }

        // Install modes
        std::optional<std::reference_wrapper<const web::json::array>> installModes = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(InstallModes));
        if (installModes.has_value())
        {
            for (auto& mode : installModes.value().get())
            {
                std::optional<std::string> modeOject = JsonHelper::GetRawStringValueFromJsonValue(mode);
                if (modeOject.has_value())
                {
                    installer.InstallModes.emplace_back(Manifest::ConvertToScopeEnum(modeOject.value()));
                }
            }
        }

        // Installer Switches
        auto& installerSwitches = installerJsonObject.at(JsonHelper::GetJsonKeyNameString(InstallerSwitches));
        installer.Switches[InstallerSwitchType::Silent] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(Silent)).value_or("");
        installer.Switches[InstallerSwitchType::SilentWithProgress] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(SilentWithProgress)).value_or("");
        installer.Switches[InstallerSwitchType::Interactive] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(Interactive)).value_or("");
        installer.Switches[InstallerSwitchType::InstallLocation] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(InstallLocation)).value_or("");
        installer.Switches[InstallerSwitchType::Log] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(Log)).value_or("");
        installer.Switches[InstallerSwitchType::Update] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(Upgrade)).value_or("");
        installer.Switches[InstallerSwitchType::Custom] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetJsonKeyNameString(Custom)).value_or("");

        // Installer SuccessCodes
        std::optional<std::reference_wrapper<const web::json::array>> installSuccessCodes = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(InstallerSuccessCodes));
        if (installSuccessCodes.has_value())
        {
            for (auto& code : installSuccessCodes.value().get())
            {
                std::optional<int> codeValue = JsonHelper::GetRawIntValueFromJsonValue(code);
                if (codeValue.has_value())
                {
                    installer.InstallerSuccessCodes.emplace_back(std::move(codeValue.value()));
                }
            }
        }

        std::optional<std::string> updateBehavior = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(UpgradeBehavior));
        if (updateBehavior.has_value())
        {
            installer.UpdateBehavior = Manifest::ConvertToUpdateBehaviorEnum(updateBehavior.value());
        }

        installer.Commands = JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(Commands));
        installer.Protocols = JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(Protocols));
        installer.FileExtensions = JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(FileExtensions));

        // Dependencies
        auto& dependenciesObject = installerJsonObject.at(JsonHelper::GetJsonKeyNameString(Dependencies));
        std::optional<Manifest::Dependency> dependency = DeserializeDependency(dependenciesObject);
        if (dependency.has_value())
        {
            installer.Dependencies = std::move(dependency.value());
        }

        installer.PackageFamilyName = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(PackageFamilyName)).value_or("");
        installer.ProductCode = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(ProductCode)).value_or("");
        installer.Capabilities = JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(Capabilities));
        installer.RestrictedCapabilities = JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetJsonKeyNameString(RestrictedCapabilities));

        return installer;
    }

    std::optional<Manifest::Dependency> ManifestDeserializer::DeserializeDependency(const web::json::value& dependenciesObject) const
    {
        if (dependenciesObject.is_null())
        {
            return {};
        }

        Manifest::Dependency dependency;

        dependency.WindowsFeatures = JsonHelper::GetRawStringArrayFromJsonNode(dependenciesObject, JsonHelper::GetJsonKeyNameString(WindowsFeatures));
        dependency.WindowsLibraries = JsonHelper::GetRawStringArrayFromJsonNode(dependenciesObject, JsonHelper::GetJsonKeyNameString(WindowsLibraries));
        dependency.ExternalDependencies = JsonHelper::GetRawStringArrayFromJsonNode(dependenciesObject, JsonHelper::GetJsonKeyNameString(ExternalDependencies));

        // Package Dependencies
        std::optional<std::reference_wrapper<const web::json::array>> packageDependencies = JsonHelper::GetRawJsonArrayFromJsonNode(dependenciesObject, JsonHelper::GetJsonKeyNameString(PackageDependencies));
        if (packageDependencies.has_value())
        {
            for (auto& packageDependency : packageDependencies.value().get())
            {
                std::optional<std::string> id = JsonHelper::GetRawStringValueFromJsonNode(packageDependency, JsonHelper::GetJsonKeyNameString(PackageIdentifier));
                if (id.has_value())
                {
                    PackageDependency pkg{ std::move(id.value()) ,  JsonHelper::GetRawStringValueFromJsonNode(packageDependency, JsonHelper::GetJsonKeyNameString(MinimumVersion)).value_or("") };
                    dependency.PackageDependencies.emplace_back(std::move(pkg));
                }
            }
        }

        return dependency;
    }
}
