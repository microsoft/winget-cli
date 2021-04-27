// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/HttpClientHelper.h"
#include "ManifestDeserializer.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/1_0/Json/CommonJsonConstants.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
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

        std::vector<Manifest::string_t> ConvertToManifestStringArray(const std::vector<std::string>& values)
        {
            std::vector<Manifest::string_t> result;
            for (const auto& value : values)
            {
                result.emplace_back(value);
            }

            return result;
        }
    }

    std::vector<Manifest::Manifest> ManifestDeserializer::Deserialize(const web::json::value& dataJsonObject) const
    {
        // Get manifest from json output.
        std::optional<std::vector<Manifest::Manifest>> manifests = DeserializeVersion(dataJsonObject);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA, !manifests);

        return manifests.value();
    }

    std::optional<std::vector<Manifest::Manifest>> ManifestDeserializer::DeserializeVersion(const web::json::value& dataJsonObject) const
    {
        if (dataJsonObject.is_null())
        {
            AICLI_LOG(Repo, Error, << "Missing json object.");
            return {};
        }

        std::vector<Manifest::Manifest> manifests;
        try
        {
            std::optional<std::reference_wrapper<const web::json::value>> manifestObject =
                JsonHelper::GetJsonValueFromNode(dataJsonObject, JsonHelper::GetUtilityString(Data));

            if (!manifestObject || manifestObject.value().get().is_null())
            {
                AICLI_LOG(Repo, Verbose, << "No manifest results returned.");
                return manifests;
            }

            auto& manifestJsonObject = manifestObject.value().get();
            std::optional<std::string> id = JsonHelper::GetRawStringValueFromJsonNode(manifestJsonObject, JsonHelper::GetUtilityString(PackageIdentifier));
            if (!JsonHelper::IsValidNonEmptyStringValue(id))
            {
                AICLI_LOG(Repo, Error, << "Missing package identifier.");
                return {};
            }

            std::optional<std::reference_wrapper<const web::json::array>> versions = JsonHelper::GetRawJsonArrayFromJsonNode(manifestJsonObject, JsonHelper::GetUtilityString(Versions));
            if (!versions || versions.value().get().size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Missing versions in package: " << id.value());
                return {};
            }

            const web::json::array versionNodes = versions.value().get();
            for (auto& versionItem : versionNodes)
            {
                Manifest::Manifest manifest;
                manifest.Id = id.value();

                std::optional<std::string> packageVersion = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetUtilityString(PackageVersion));
                if (!JsonHelper::IsValidNonEmptyStringValue(packageVersion))
                {
                    AICLI_LOG(Repo, Error, << "Missing package version in package: " << manifest.Id);
                    return {};
                }
                manifest.Version = std::move(packageVersion.value());

                manifest.Channel = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetUtilityString(Channel)).value_or("");

                // Default locale
                std::optional<std::reference_wrapper<const web::json::value>> defaultLocale =
                    JsonHelper::GetJsonValueFromNode(versionItem, JsonHelper::GetUtilityString(DefaultLocale));
                if (!defaultLocale)
                {
                    AICLI_LOG(Repo, Error, << "Missing default locale in package: " << manifest.Id);
                    return {};
                }
                else
                {
                    std::optional<Manifest::ManifestLocalization> defaultLocaleObject = DeserializeLocale(defaultLocale.value().get());
                    if (!defaultLocaleObject)
                    {
                        AICLI_LOG(Repo, Error, << "Missing default locale in package: " << manifest.Id);
                        return {};
                    }
                    manifest.DefaultLocalization = std::move(defaultLocaleObject.value());

                    // Moniker is in Default locale
                    manifest.Moniker = JsonHelper::GetRawStringValueFromJsonNode(defaultLocale.value().get(), JsonHelper::GetUtilityString(Moniker)).value_or("");
                }

                // Installers
                std::optional<std::reference_wrapper<const web::json::array>> installers = JsonHelper::GetRawJsonArrayFromJsonNode(versionItem, JsonHelper::GetUtilityString(Installers));
                if (!installers || installers.value().get().size() == 0)
                {
                    AICLI_LOG(Repo, Error, << "Missing installers in package: " << manifest.Id);
                    return {};
                }

                for (auto& installer : installers.value().get())
                {
                    std::optional<Manifest::ManifestInstaller> installerObject = DeserializeInstaller(installer);
                    if (installerObject)
                    {
                        manifest.Installers.emplace_back(std::move(installerObject.value()));
                    }
                }

                if (manifest.Installers.size() == 0)
                {
                    AICLI_LOG(Repo, Error, << "Missing valid installers in package: " << manifest.Id);
                    return {};
                }

                // Other locales
                std::optional<std::reference_wrapper<const web::json::array>> locales = JsonHelper::GetRawJsonArrayFromJsonNode(versionItem, JsonHelper::GetUtilityString(Locales));
                if (locales)
                {
                    for (auto& locale : locales.value().get())
                    {
                        std::optional<Manifest::ManifestLocalization> localeObject = DeserializeLocale(locale);
                        if (localeObject)
                        {
                            manifest.Localizations.emplace_back(std::move(localeObject.value()));
                        }
                    }
                }

                manifests.emplace_back(std::move(manifest));
            }

            return manifests;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing manifest. Reason: " << e.what());
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing manifest...");
        }

        return {};
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        if (localeJsonObject.is_null())
        {
            return {};
        }

        Manifest::ManifestLocalization locale;
        std::optional<std::string> packageLocale = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(PackageLocale));
        if (!JsonHelper::IsValidNonEmptyStringValue(packageLocale))
        {
            AICLI_LOG(Repo, Error, << "Missing package locale.");
            return {};
        }
        locale.Locale = std::move(packageLocale.value());

        std::optional<std::string> packageName = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(PackageName));
        if (!JsonHelper::IsValidNonEmptyStringValue(packageName))
        {
            AICLI_LOG(Repo, Error, << "Missing package name.");
            return {};
        }
        locale.Add<AppInstaller::Manifest::Localization::PackageName>(std::move(packageName.value()));

        std::optional<std::string> publisher = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(Publisher));
        if (!JsonHelper::IsValidNonEmptyStringValue(publisher))
        {
            AICLI_LOG(Repo, Error, << "Missing publisher.");
            return {};
        }
        locale.Add<AppInstaller::Manifest::Localization::Publisher>(std::move(publisher.value()));

        std::optional<std::string> shortDescription = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(ShortDescription));
        if (!JsonHelper::IsValidNonEmptyStringValue(shortDescription))
        {
            AICLI_LOG(Repo, Error, << "Missing short description.");
            return {};
        }
        locale.Add<AppInstaller::Manifest::Localization::ShortDescription>(std::move(shortDescription.value()));

        locale.Add<AppInstaller::Manifest::Localization::PublisherUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(PublisherUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::PublisherSupportUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(PublisherSupportUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::PrivacyUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(PrivacyUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Author>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(Author)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::PackageUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(PackageUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::License>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(License)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::LicenseUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(LicenseUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Copyright>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(Copyright)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::CopyrightUrl>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(CopyrightUrl)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Description>(JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(Description)).value_or(""));
        locale.Add<AppInstaller::Manifest::Localization::Tags>(ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(Tags))));

        return locale;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        if (installerJsonObject.is_null())
        {
            return {};
        }

        Manifest::ManifestInstaller installer;
        std::optional<std::string> url = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerUrl));
        if (!JsonHelper::IsValidNonEmptyStringValue(url))
        {
            AICLI_LOG(Repo, Error, << "Missing installer url.");
            return {};
        }
        installer.Url = std::move(url.value());

        std::optional<std::string> sha256 = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerSha256));
        if (!JsonHelper::IsValidNonEmptyStringValue(sha256))
        {
            AICLI_LOG(Repo, Error, << "Missing installer SHA256.");
            return {};
        }
        installer.Sha256 = Utility::SHA256::ConvertToBytes(sha256.value());

        std::optional<std::string> arch = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(Architecture));
        if (!JsonHelper::IsValidNonEmptyStringValue(arch))
        {
            AICLI_LOG(Repo, Error, << "Missing installer architecture.");
            return {};
        }
        installer.Arch = Utility::ConvertToArchitectureEnum(arch.value());

        std::optional<std::string> installerType = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerType));
        if (!JsonHelper::IsValidNonEmptyStringValue(installerType))
        {
            AICLI_LOG(Repo, Error, << "Missing installer type.");
            return {};
        }
        installer.InstallerType = Manifest::ConvertToInstallerTypeEnum(installerType.value());
        installer.Locale = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerLocale)).value_or("");

        // platform
        std::optional<std::reference_wrapper<const web::json::array>> platforms = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(Platform));
        if (platforms)
        {
            for (auto& platform : platforms.value().get())
            {
                std::optional<std::string> platformValue = JsonHelper::GetRawStringValueFromJsonValue(platform);
                if (platformValue)
                {
                    installer.Platform.emplace_back(Manifest::ConvertToPlatformEnum(platformValue.value()));
                }
            }
        }

        installer.MinOSVersion = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(MinimumOSVersion)).value_or("");
        std::optional<std::string> scope = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(Scope));
        if (scope)
        {
            installer.Scope = Manifest::ConvertToScopeEnum(scope.value());
        }

        std::optional<std::string> signatureSha256 = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(SignatureSha256));
        if (signatureSha256)
        {
            installer.SignatureSha256 = Utility::SHA256::ConvertToBytes(signatureSha256.value());
        }

        // Install modes
        std::optional<std::reference_wrapper<const web::json::array>> installModes = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallModes));
        if (installModes)
        {
            for (auto& mode : installModes.value().get())
            {
                std::optional<std::string> modeObject = JsonHelper::GetRawStringValueFromJsonValue(mode);
                if (modeObject)
                {
                    installer.InstallModes.emplace_back(Manifest::ConvertToInstallModeEnum(modeObject.value()));
                }
            }
        }

        // Installer Switches
        std::optional<std::reference_wrapper<const web::json::value>> switches =
            JsonHelper::GetJsonValueFromNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerSwitches));
        if (switches)
        {
            auto& installerSwitches = switches.value().get();
            installer.Switches[InstallerSwitchType::Silent] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(Silent)).value_or("");
            installer.Switches[InstallerSwitchType::SilentWithProgress] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(SilentWithProgress)).value_or("");
            installer.Switches[InstallerSwitchType::Interactive] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(Interactive)).value_or("");
            installer.Switches[InstallerSwitchType::InstallLocation] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(InstallLocation)).value_or("");
            installer.Switches[InstallerSwitchType::Log] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(Log)).value_or("");
            installer.Switches[InstallerSwitchType::Update] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(Upgrade)).value_or("");
            installer.Switches[InstallerSwitchType::Custom] = JsonHelper::GetRawStringValueFromJsonNode(installerSwitches, JsonHelper::GetUtilityString(Custom)).value_or("");
        }

        // Installer SuccessCodes
        std::optional<std::reference_wrapper<const web::json::array>> installSuccessCodes = JsonHelper::GetRawJsonArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(InstallerSuccessCodes));
        if (installSuccessCodes)
        {
            for (auto& code : installSuccessCodes.value().get())
            {
                std::optional<int> codeValue = JsonHelper::GetRawIntValueFromJsonValue(code);
                if (codeValue)
                {
                    installer.InstallerSuccessCodes.emplace_back(std::move(codeValue.value()));
                }
            }
        }

        std::optional<std::string> updateBehavior = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(UpgradeBehavior));
        if (updateBehavior)
        {
            installer.UpdateBehavior = Manifest::ConvertToUpdateBehaviorEnum(updateBehavior.value());
        }

        installer.Commands = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(Commands)));
        installer.Protocols = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(Protocols)));
        installer.FileExtensions = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(FileExtensions)));

        // Dependencies
        std::optional<std::reference_wrapper<const web::json::value>> dependenciesObject =
            JsonHelper::GetJsonValueFromNode(installerJsonObject, JsonHelper::GetUtilityString(Dependencies));
        if (dependenciesObject)
        {
            std::optional<Manifest::Dependency> dependency = DeserializeDependency(dependenciesObject.value().get());
            if (dependency)
            {
                installer.Dependencies = std::move(dependency.value());
            }
        }

        installer.PackageFamilyName = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(PackageFamilyName)).value_or("");
        installer.ProductCode = JsonHelper::GetRawStringValueFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(ProductCode)).value_or("");
        installer.Capabilities = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(Capabilities)));
        installer.RestrictedCapabilities = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(installerJsonObject, JsonHelper::GetUtilityString(RestrictedCapabilities)));

        return installer;
    }

    std::optional<Manifest::Dependency> ManifestDeserializer::DeserializeDependency(const web::json::value& dependenciesObject) const
    {
        if (dependenciesObject.is_null())
        {
            return {};
        }

        Manifest::Dependency dependency;

        dependency.WindowsFeatures = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(dependenciesObject, JsonHelper::GetUtilityString(WindowsFeatures)));
        dependency.WindowsLibraries = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(dependenciesObject, JsonHelper::GetUtilityString(WindowsLibraries)));
        dependency.ExternalDependencies = ConvertToManifestStringArray(JsonHelper::GetRawStringArrayFromJsonNode(dependenciesObject, JsonHelper::GetUtilityString(ExternalDependencies)));

        // Package Dependencies
        std::optional<std::reference_wrapper<const web::json::array>> packageDependencies = JsonHelper::GetRawJsonArrayFromJsonNode(dependenciesObject, JsonHelper::GetUtilityString(PackageDependencies));
        if (packageDependencies)
        {
            for (auto& packageDependency : packageDependencies.value().get())
            {
                std::optional<std::string> id = JsonHelper::GetRawStringValueFromJsonNode(packageDependency, JsonHelper::GetUtilityString(PackageIdentifier));
                if (id)
                {
                    PackageDependency pkg{ std::move(id.value()) ,  JsonHelper::GetRawStringValueFromJsonNode(packageDependency, JsonHelper::GetUtilityString(MinimumVersion)).value_or("") };
                    dependency.PackageDependencies.emplace_back(std::move(pkg));
                }
            }
        }

        return dependency;
    }
}
