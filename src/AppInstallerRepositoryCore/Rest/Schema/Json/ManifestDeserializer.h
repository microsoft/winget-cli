// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "cpprest/json.h"

namespace AppInstaller::Repository::Rest::Schema::Json
{
    // Manifest Deserializer.
    struct ManifestDeserializer
    {
        // Gets the manifest from the given json object
        std::optional<Manifest::Manifest> Deserialize(const web::json::value& manifestJsonObject) const;

    protected:
        std::optional<Manifest::Manifest> DeserializeVersion(const web::json::value& versionJsonObject) const;

        std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& localeJsonObject) const;

        std::optional<Manifest::ManifestInstaller> DeserializeInstaller(const web::json::value& intallerJsonObject) const;

        std::optional<Manifest::Dependency> DeserializeDependency(const web::json::value& dependenciesJsonObject) const;
    };
}