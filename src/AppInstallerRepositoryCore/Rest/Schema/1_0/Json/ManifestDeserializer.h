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
        // Gets the manifest from the given json object
        std::vector<Manifest::Manifest> Deserialize(const web::json::value& dataJsonObject) const;

    protected:

        template <Manifest::Localization L>
        inline void TryParseStringLocaleField(Manifest::ManifestLocalization& manifestLocale, const web::json::value& localeJsonObject, std::string_view localeJsonFieldName) const
        {
            auto value = JsonHelper::GetRawStringValueFromJsonNode(localeJsonObject, JsonHelper::GetUtilityString(localeJsonFieldName));

            if (JsonHelper::IsValidNonEmptyStringValue(value))
            {
                manifestLocale.Add<L>(value.value());
            }
        }

        std::optional<std::vector<Manifest::Manifest>> DeserializeVersion(const web::json::value& dataJsonObject) const;

        virtual std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const;

        virtual std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const;

        std::optional<Manifest::DependencyList> DeserializeDependency(const web::json::value& dependenciesJsonObject) const;

        virtual Manifest::InstallerTypeEnum ConvertToInstallerType(std::string_view in) const;

        std::vector<Manifest::string_t> ConvertToManifestStringArray(const std::vector<std::string>& values) const;
    };
}
