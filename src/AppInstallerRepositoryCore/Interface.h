// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "IRestClient.h"
#include <memory>
#include <vector>


namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public IRestClient
    {
        IRestClient::SearchResult Search(std::string m_restApiUri, const SearchRequest& request) const override;
        std::optional<std::string> GetPropertyFromVersion(const std::string& manifest, PackageVersionProperty property) const override;
        //std::vector<std::string> GetMultiPropertyByManifestId(SQLite::rowid_t manifestId, PackageVersionMultiProperty property) const override;
        std::optional<std::string> GetVersionFromPackage(const Manifest::Manifest& manifest, std::string_view version, std::string_view channel) const override;
        std::vector<Utility::VersionAndChannel> GetVersionKeysFromPackage(const Manifest::Manifest& manifest) const override;

        //// Version 1.1
        //MetadataResult GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const override;
        //void SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value) override;
    };
}