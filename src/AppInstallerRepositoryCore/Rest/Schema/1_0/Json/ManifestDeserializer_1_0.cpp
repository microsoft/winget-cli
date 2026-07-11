// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/IRestClient.h"
#include "ManifestDeserializer.h"
#include <winget/HttpClientHelper.h>
#include <winget/JsonUtil.h>

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

        void TryParseInstallerSwitchField(
            std::map<InstallerSwitchType, Utility::NormalizedString>& installerSwitches,
            InstallerSwitchType switchType,
            const web::json::value& switchesJsonObject,
            std::string_view switchJsonFieldName)
        {
            auto value = JSON::GetRawStringValueFromJsonNode(switchesJsonObject, JSON::GetUtilityString(switchJsonFieldName));

            if (JSON::IsValidNonEmptyStringValue(value))
            {
                installerSwitches[switchType] = value.value();
            }
        }
    }

    std::vector<Manifest::Manifest> ManifestDeserializer::Deserialize(const web::json::value& responseJsonObject) const
    {
        if (responseJsonObject.is_null())
        {
            AICLI_LOG(Repo, Error, << "Missing json object.");
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
        }

        try
        {
            std::optional<std::reference_wrapper<const web::json::value>> manifestObject =
                JSON::GetJsonValueFromNode(responseJsonObject, JSON::GetUtilityString(Data));

            if (!manifestObject || manifestObject.value().get().is_null())
            {
                AICLI_LOG(Repo, Verbose, << "No manifest results returned.");
                return {};
            }

            return DeserializeData(manifestObject.value());
        }
        catch (const wil::ResultException&)
        {
            throw;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing manifest. Reason: " << e.what());
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "Error encountered while deserializing manifest...");
        }

        // If we make it here, there was an exception above that we didn't throw.
        // This will convert it into our standard error.
        THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
    }

    std::vector<Manifest::Manifest> ManifestDeserializer::DeserializeData(const web::json::value& dataJsonObject) const
    {
        THROW_HR_IF(E_INVALIDARG, dataJsonObject.is_null());

        std::vector<Manifest::Manifest> manifests;

        std::optional<std::string> id = JSON::GetRawStringValueFromJsonNode(dataJsonObject, JSON::GetUtilityString(PackageIdentifier));
        if (!JSON::IsValidNonEmptyStringValue(id))
        {
            AICLI_LOG(Repo, Error, << "Missing package identifier.");
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
        }

        std::optional<std::reference_wrapper<const web::json::array>> versions = JSON::GetRawJsonArrayFromJsonNode(dataJsonObject, JSON::GetUtilityString(Versions));
        if (!versions || versions.value().get().size() == 0)
        {
            AICLI_LOG(Repo, Error, << "Missing versions in package: " << id.value());
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
        }

        for (auto& versionItem : versions.value().get())
        {
            Manifest::Manifest manifest;

            manifest.ManifestVersion = GetManifestVersion();

            manifest.Id = id.value();

            std::optional<std::string> packageVersion = JSON::GetRawStringValueFromJsonNode(versionItem, JSON::GetUtilityString(PackageVersion));
            if (!JSON::IsValidNonEmptyStringValue(packageVersion))
            {
                AICLI_LOG(Repo, Error, << "Missing package version in package: " << manifest.Id);
                THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
            }
            manifest.Version = std::move(packageVersion.value());

            manifest.Channel = JSON::GetRawStringValueFromJsonNode(versionItem, JSON::GetUtilityString(Channel)).value_or("");

            // Default locale
            std::optional<std::reference_wrapper<const web::json::value>> defaultLocale =
                JSON::GetJsonValueFromNode(versionItem, JSON::GetUtilityString(DefaultLocale));
            if (!defaultLocale)
            {
                AICLI_LOG(Repo, Error, << "Missing default locale in package: " << manifest.Id);
                THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
            }
            else
            {
                std::optional<Manifest::ManifestLocalization> defaultLocaleObject = DeserializeLocale(defaultLocale.value().get());
                if (!defaultLocaleObject)
                {
                    AICLI_LOG(Repo, Error, << "Missing default locale in package: " << manifest.Id);
                    THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
                }

                if (!defaultLocaleObject.value().Contains(Manifest::Localization::PackageName) ||
                    !defaultLocaleObject.value().Contains(Manifest::Localization::Publisher) ||
                    !defaultLocaleObject.value().Contains(Manifest::Localization::ShortDescription))
                {
                    AICLI_LOG(Repo, Error, << "Missing PackageName, Publisher or ShortDescription in default locale: " << manifest.Id);
                    THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
                }

                manifest.DefaultLocalization = std::move(defaultLocaleObject.value());

                // Moniker is in Default locale
                manifest.Moniker = JSON::GetRawStringValueFromJsonNode(defaultLocale.value().get(), JSON::GetUtilityString(Moniker)).value_or("");
            }

            // Installers
            std::optional<std::reference_wrapper<const web::json::array>> installers = JSON::GetRawJsonArrayFromJsonNode(versionItem, JSON::GetUtilityString(Installers));
            if (!installers || installers.value().get().size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Missing installers in package: " << manifest.Id);
                THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
            }

            for (auto& installer : installers.value().get())
            {
                std::optional<Manifest::ManifestInstaller> installerObject = DeserializeInstaller(installer);
                if (installerObject)
                {
                    // Merge default switches after parsing.
                    auto defaultSwitches = Manifest::GetDefaultKnownSwitches(installerObject->EffectiveInstallerType());
                    for (auto const& defaultSwitch : defaultSwitches)
                    {
                        if (installerObject->Switches.find(defaultSwitch.first) == installerObject->Switches.end())
                        {
                            installerObject->Switches[defaultSwitch.first] = defaultSwitch.second;
                        }
                    }

                    manifest.Installers.emplace_back(std::move(installerObject.value()));
                }
            }

            if (manifest.Installers.size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Missing valid installers in package: " << manifest.Id);
                THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA);
            }

            // Other locales
            std::optional<std::reference_wrapper<const web::json::array>> locales = JSON::GetRawJsonArrayFromJsonNode(versionItem, JSON::GetUtilityString(Locales));
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

    std::vector<Manifest::AppsAndFeaturesEntry> ManifestDeserializer::DeserializeAppsAndFeaturesEntries(const web::json::array&) const
    {
        return {};
    }

    std::optional<Manifest::InstallationMetadataInfo> ManifestDeserializer::DeserializeInstallationMetadata(const web::json::value&) const
    {
        return {};
    }

    std::optional<Manifest::ManifestLocalization> ManifestDeserializer::DeserializeLocale(const web::json::value& localeJsonObject) const
    {
        if (localeJsonObject.is_null())
        {
            return {};
        }

        Manifest::ManifestLocalization locale;
        std::optional<std::string> packageLocale = JSON::GetRawStringValueFromJsonNode(localeJsonObject, JSON::GetUtilityString(PackageLocale));
        if (!JSON::IsValidNonEmptyStringValue(packageLocale))
        {
            AICLI_LOG(Repo, Error, << "Missing package locale.");
            return {};
        }
        locale.Locale = std::move(packageLocale.value());

        TryParseStringLocaleField<Manifest::Localization::PackageName>(locale, localeJsonObject, PackageName);
        TryParseStringLocaleField<Manifest::Localization::Publisher>(locale, localeJsonObject, Publisher);
        TryParseStringLocaleField<Manifest::Localization::ShortDescription>(locale, localeJsonObject, ShortDescription);
        TryParseStringLocaleField<Manifest::Localization::PublisherUrl>(locale, localeJsonObject, PublisherUrl);
        TryParseStringLocaleField<Manifest::Localization::PublisherSupportUrl>(locale, localeJsonObject, PublisherSupportUrl);
        TryParseStringLocaleField<Manifest::Localization::PrivacyUrl>(locale, localeJsonObject, PrivacyUrl);
        TryParseStringLocaleField<Manifest::Localization::Author>(locale, localeJsonObject, Author);
        TryParseStringLocaleField<Manifest::Localization::PackageUrl>(locale, localeJsonObject, PackageUrl);
        TryParseStringLocaleField<Manifest::Localization::License>(locale, localeJsonObject, License);
        TryParseStringLocaleField<Manifest::Localization::LicenseUrl>(locale, localeJsonObject, LicenseUrl);
        TryParseStringLocaleField<Manifest::Localization::Copyright>(locale, localeJsonObject, Copyright);
        TryParseStringLocaleField<Manifest::Localization::CopyrightUrl>(locale, localeJsonObject, CopyrightUrl);
        TryParseStringLocaleField<Manifest::Localization::Description>(locale, localeJsonObject, Description);

        auto tags = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(localeJsonObject, JSON::GetUtilityString(Tags)));
        if (!tags.empty())
        {
            locale.Add<AppInstaller::Manifest::Localization::Tags>(tags);
        }

        return locale;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        if (installerJsonObject.is_null())
        {
            return {};
        }

        Manifest::ManifestInstaller installer;

        installer.Url = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerUrl)).value_or("");

        std::optional<std::string> sha256 = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerSha256));
        if (JSON::IsValidNonEmptyStringValue(sha256))
        {
            installer.Sha256 = Utility::SHA256::ConvertToBytes(sha256.value());
        }

        std::optional<std::string> arch = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(Architecture));
        if (!JSON::IsValidNonEmptyStringValue(arch))
        {
            AICLI_LOG(Repo, Error, << "Missing installer architecture.");
            return {};
        }
        installer.Arch = Utility::ConvertToArchitectureEnum(arch.value());

        std::optional<std::string> installerType = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerType));
        if (!JSON::IsValidNonEmptyStringValue(installerType))
        {
            AICLI_LOG(Repo, Error, << "Missing installer type.");
            return {};
        }
        installer.BaseInstallerType = ConvertToInstallerType(installerType.value());
        installer.Locale = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerLocale)).value_or("");

        // platform
        std::optional<std::reference_wrapper<const web::json::array>> platforms = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(Platform));
        if (platforms)
        {
            for (auto& platform : platforms.value().get())
            {
                std::optional<std::string> platformValue = JSON::GetRawStringValueFromJsonValue(platform);
                if (platformValue)
                {
                    installer.Platform.emplace_back(Manifest::ConvertToPlatformEnum(platformValue.value()));
                }
            }
        }

        installer.MinOSVersion = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(MinimumOSVersion)).value_or("");
        std::optional<std::string> scope = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(Scope));
        if (scope)
        {
            installer.Scope = Manifest::ConvertToScopeEnum(scope.value());
        }

        std::optional<std::string> signatureSha256 = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(SignatureSha256));
        if (signatureSha256)
        {
            installer.SignatureSha256 = Utility::SHA256::ConvertToBytes(signatureSha256.value());
        }

        // Install modes
        std::optional<std::reference_wrapper<const web::json::array>> installModes = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallModes));
        if (installModes)
        {
            for (auto& mode : installModes.value().get())
            {
                std::optional<std::string> modeObject = JSON::GetRawStringValueFromJsonValue(mode);
                if (modeObject)
                {
                    installer.InstallModes.emplace_back(Manifest::ConvertToInstallModeEnum(modeObject.value()));
                }
            }
        }

        // Installer Switches
        std::optional<std::reference_wrapper<const web::json::value>> switches =
            JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(InstallerSwitches));
        if (switches)
        {
            installer.Switches = DeserializeInstallerSwitches(switches.value().get());
        }

        // Installer SuccessCodes
        std::optional<std::reference_wrapper<const web::json::array>> installSuccessCodes = JSON::GetRawJsonArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(InstallerSuccessCodes));
        if (installSuccessCodes)
        {
            for (auto& code : installSuccessCodes.value().get())
            {
                std::optional<int> codeValue = JSON::GetRawIntValueFromJsonValue(code);
                if (codeValue)
                {
                    installer.InstallerSuccessCodes.emplace_back(std::move(codeValue.value()));
                }
            }
        }

        std::optional<std::string> updateBehavior = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(UpgradeBehavior));
        if (updateBehavior)
        {
            installer.UpdateBehavior = ConvertToUpdateBehavior(updateBehavior.value());
        }

        installer.Commands = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(Commands)));
        installer.Protocols = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(Protocols)));
        installer.FileExtensions = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(FileExtensions)));

        // Dependencies
        std::optional<std::reference_wrapper<const web::json::value>> dependenciesObject =
            JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(Dependencies));
        if (dependenciesObject)
        {
            std::optional<Manifest::DependencyList> dependencyList = DeserializeDependency(dependenciesObject.value().get());
            if (dependencyList)
            {
                installer.Dependencies = std::move(dependencyList.value());
            }
        }

        installer.PackageFamilyName = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(PackageFamilyName)).value_or("");
        installer.ProductCode = JSON::GetRawStringValueFromJsonNode(installerJsonObject, JSON::GetUtilityString(ProductCode)).value_or("");
        installer.Capabilities = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(Capabilities)));
        installer.RestrictedCapabilities = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(installerJsonObject, JSON::GetUtilityString(RestrictedCapabilities)));

        return installer;
    }

    std::map<Manifest::InstallerSwitchType, Manifest::string_t> ManifestDeserializer::DeserializeInstallerSwitches(const web::json::value& installerSwitchesJsonObject) const
    {
        std::map<Manifest::InstallerSwitchType, Manifest::string_t> installerSwitches;

        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::Silent, installerSwitchesJsonObject, Silent);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::SilentWithProgress, installerSwitchesJsonObject, SilentWithProgress);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::Interactive, installerSwitchesJsonObject, Interactive);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::InstallLocation, installerSwitchesJsonObject, InstallLocation);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::Log, installerSwitchesJsonObject, Log);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::Update, installerSwitchesJsonObject, Upgrade);
        TryParseInstallerSwitchField(installerSwitches, InstallerSwitchType::Custom, installerSwitchesJsonObject, Custom);

        return installerSwitches;
    }

    std::optional<Manifest::DependencyList> ManifestDeserializer::DeserializeDependency(const web::json::value& dependenciesObject) const
    {
        if (dependenciesObject.is_null())
        {
            return {};
        }

        Manifest::DependencyList dependencyList;

        auto wfIds = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(dependenciesObject, JSON::GetUtilityString(WindowsFeatures)));
        for (auto&& id : wfIds)
        {
            dependencyList.Add(Dependency(DependencyType::WindowsFeature, std::move(id)));
        };

        const auto& wlIds = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(dependenciesObject, JSON::GetUtilityString(WindowsLibraries)));
        for (auto id : wlIds)
        {
            dependencyList.Add(Dependency(DependencyType::WindowsLibrary, id));
        };

        const auto& extIds = ConvertToManifestStringArray(JSON::GetRawStringArrayFromJsonNode(dependenciesObject, JSON::GetUtilityString(ExternalDependencies)));
        for (auto id : extIds)
        {
            dependencyList.Add(Dependency(DependencyType::External, id));
        };

        // Package Dependencies
        std::optional<std::reference_wrapper<const web::json::array>> packageDependencies = JSON::GetRawJsonArrayFromJsonNode(dependenciesObject, JSON::GetUtilityString(PackageDependencies));
        if (packageDependencies)
        {
            for (auto& packageDependency : packageDependencies.value().get())
            {
                std::optional<std::string> id = JSON::GetRawStringValueFromJsonNode(packageDependency, JSON::GetUtilityString(PackageIdentifier));
                if (id)
                {
                    Dependency pkg{ DependencyType::Package, std::move(id.value()) ,  JSON::GetRawStringValueFromJsonNode(packageDependency, JSON::GetUtilityString(MinimumVersion)).value_or("") };
                    dependencyList.Add(std::move(pkg));
                }
            }
        }

        return dependencyList;
    }

    Manifest::InstallerTypeEnum ManifestDeserializer::ConvertToInstallerType(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "inno")
        {
            return InstallerTypeEnum::Inno;
        }
        else if (inStrLower == "wix")
        {
            return InstallerTypeEnum::Wix;
        }
        else if (inStrLower == "msi")
        {
            return InstallerTypeEnum::Msi;
        }
        else if (inStrLower == "nullsoft")
        {
            return InstallerTypeEnum::Nullsoft;
        }
        else if (inStrLower == "zip")
        {
            return InstallerTypeEnum::Zip;
        }
        else if (inStrLower == "appx" || inStrLower == "msix")
        {
            return InstallerTypeEnum::Msix;
        }
        else if (inStrLower == "exe")
        {
            return InstallerTypeEnum::Exe;
        }
        else if (inStrLower == "burn")
        {
            return InstallerTypeEnum::Burn;
        }

        return InstallerTypeEnum::Unknown;
    }

    Manifest::UpdateBehaviorEnum ManifestDeserializer::ConvertToUpdateBehavior(std::string_view in) const
    {
        std::string inStrLower = Utility::ToLower(in);

        if (inStrLower == "install")
        {
            return UpdateBehaviorEnum::Install;
        }
        else if (inStrLower == "uninstallprevious")
        {
            return UpdateBehaviorEnum::UninstallPrevious;
        }

        return UpdateBehaviorEnum::Unknown;
    }

    std::vector<Manifest::string_t> ManifestDeserializer::ConvertToManifestStringArray(const std::vector<std::string>& values) const
    {
        std::vector<Manifest::string_t> result;
        for (const auto& value : values)
        {
            result.emplace_back(value);
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1;
    }
}
