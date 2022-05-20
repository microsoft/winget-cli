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

        virtual std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const;

        virtual std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const;

        std::optional<Manifest::DependencyList> DeserializeDependency(const web::json::value& dependenciesJsonObject) const;

        virtual Manifest::InstallerTypeEnum ConvertToInstallerType(std::string_view in) const;

        std::vector<Manifest::string_t> ConvertToManifestStringArray(const std::vector<std::string>& values) const;
    };
}
