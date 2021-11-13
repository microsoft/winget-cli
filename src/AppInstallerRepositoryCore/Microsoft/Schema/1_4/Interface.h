// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_3/Interface.h"
#include "Microsoft/Schema/1_2/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_4
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_3::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection, CreateOptions options) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) override;
        void PrepareForPackaging(SQLite::Connection& connection, bool vacuum) override;

        std::map<Manifest::Dependency, SQLite::rowid_t> GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId) const override;
        std::vector<std::pair<Manifest::Manifest, Utility::Version>> GetDependenciesByPackageId(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId) const override;
    };
}