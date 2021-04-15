// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Manifest.h>
#include <cpprest/json.h>

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer
    {
        // Gets the manifest from the given json object
        std::vector<Manifest::Manifest> Deserialize(const web::json::value& dataJsonObject) const;

    protected:
        std::optional<std::vector<Manifest::Manifest>> DeserializeVersion(const web::json::value& dataJsonObject) const;

        std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const;

        std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& installerJsonObject) const;

        std::optional<Manifest::Dependency> DeserializeDependency(const web::json::value& dependenciesJsonObject) const;
    };
}
