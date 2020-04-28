// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public SQLiteIndexBase
    {
        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;
        void AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        bool UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        void RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        void PrepareForPackaging(SQLite::Connection& connection) override;
        SearchResult Search(SQLite::Connection& connection, const SearchRequest& request) override;
        std::optional<std::string> GetIdStringById(SQLite::Connection& connection, SQLite::rowid_t id) override;
        std::optional<std::string> GetNameStringById(SQLite::Connection& connection, SQLite::rowid_t id) override;
        std::optional<std::string> GetPathStringByKey(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) override;
        std::vector<Utility::VersionAndChannel> GetVersionsById(SQLite::Connection& connection, SQLite::rowid_t id) override;
    };
}
