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

        // Parses the manifests from the REST response object root.
        // May potentially contain multiple versions of the same package.
        std::vector<Manifest::Manifest> Deserialize(const web::json::value& response) const;

        // Parses the manifests from the Data field of the REST response object.
        // May potentially contain multiple versions of the same package.
        std::vector<Manifest::Manifest> ParseData(const web::json::value& data) const;

        // Parses the AppsAndFeaturesEntries node, returning the set of values below it.
        std::vector<Manifest::AppsAndFeaturesEntry> ParseAppsAndFeaturesEntries(const web::json::array& data) const;

    private:
        struct impl;
        std::unique_ptr<impl> m_pImpl;
    };
}
