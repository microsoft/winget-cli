// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_3/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_4
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_3::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection, CreateOptions options) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;
        void PrepareForPackaging(SQLite::Connection& connection, bool vacuum) override;

        std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId) const override;
        std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId) const override;

        // Version 1.7
        void DropTables(SQLite::Connection& connection) override;

    protected:
        bool NotNeeded(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id) const override;

    private:
        // Semantic check to validate dependencies with min versions are satisfied.
        bool ValidateDependenciesWithMinVersions(const SQLite::Connection& connection, bool log) const;
    };
}
