// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/JsonUtil.h>
#include <winget/Manifest.h>

#include <memory>
#include <vector>

namespace AppInstaller::Repository::JSON
{
    // Exposes functions for parsing JSON REST responses to manifest requests.
    struct ManifestJSONParser
    {
        ManifestJSONParser(const Utility::Version& responseSchemaVersion);

        ManifestJSONParser(const ManifestJSONParser&) = delete;
        ManifestJSONParser& operator=(const ManifestJSONParser&) = delete;

        ManifestJSONParser(ManifestJSONParser&&) noexcept;
        ManifestJSONParser& operator=(ManifestJSONParser&&) noexcept;

        ~ManifestJSONParser();

        // Deserializes the manifests from the REST response object root.
        // May potentially contain multiple versions of the same package.
        std::vector<Manifest::Manifest> Deserialize(const web::json::value& response) const;

        // Deserializes the manifests from the Data field of the REST response object.
        // May potentially contain multiple versions of the same package.
        std::vector<Manifest::Manifest> DeserializeData(const web::json::value& data) const;

        // Deserializes the AppsAndFeaturesEntries node, returning the set of values below it.
        std::vector<Manifest::AppsAndFeaturesEntry> DeserializeAppsAndFeaturesEntries(const web::json::array& data) const;

        // Deserializes the locale node; returning an object if a proper locale was found.
        std::optional<Manifest::ManifestLocalization> DeserializeLocale(const web::json::value& locale) const;

        // Deserializes the InstallationMetadata node; returning an object if a proper InstallationMetadata was found.
        std::optional<Manifest::InstallationMetadataInfo> DeserializeInstallationMetadata(const web::json::value& installationMetadata) const;

    private:
        struct impl;
        std::unique_ptr<impl> m_pImpl;
    };
}
