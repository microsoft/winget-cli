// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <cpprest/json.h>
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer
    {
        // Gets the manifest from the given json object received from a REST request
        std::vector<Manifest::Manifest> Deserialize(const web::json::value& responseJsonObject) const;

        // Gets the manifest from the given json Data field
        std::vector<Manifest::Manifest> DeserializeData(const web::json::value& dataJsonObject) const;

        // Deserializes the AppsAndFeaturesEntries node, returning the set of values below it.
        virtual std::vector<Manifest::AppsAndFeaturesEntry> DeserializeAppsAndFeaturesEntries(const web::json::array& entries) const;

        // Deserializes the locale; requires that the PackageLocale be set to return an object.
        virtual std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const;

        // Deserializes the locale; requires that the PackageLocale be set to return an object.
        virtual std::optional<Manifest::InstallationMetadataInfo> DeserializeInstallationMetadata(const web::json::value& installationMetadataJsonObject) const;

    protected:

        template <Manifest::Localization L>
        inline void TryParseStringLocaleField(Manifest::ManifestLocalization& manifestLocale, const web::json::value& localeJsonObject, std::string_view localeJsonFieldName) const
        {
            auto value = AppInstaller::JSON::GetRawStringValueFromJsonNode(localeJsonObject, AppInstaller::JSON::GetUtilityString(localeJsonFieldName));

            if (AppInstaller::JSON::IsValidNonEmptyStringValue(value))
            {
                manifestLocale.Add<L>(value.value());
            }
        }

        virtual std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const;

        virtual std::map<Manifest::InstallerSwitchType, Manifest::string_t> DeserializeInstallerSwitches(const web::json::value& installerSwitchesJsonObject) const;

        std::optional<Manifest::DependencyList> DeserializeDependency(const web::json::value& dependenciesJsonObject) const;

        virtual Manifest::InstallerTypeEnum ConvertToInstallerType(std::string_view in) const;

        virtual Manifest::UpdateBehaviorEnum ConvertToUpdateBehavior(std::string_view in) const;

        std::vector<Manifest::string_t> ConvertToManifestStringArray(const std::vector<std::string>& values) const;

        virtual Manifest::ManifestVer GetManifestVersion() const;
    };
}
