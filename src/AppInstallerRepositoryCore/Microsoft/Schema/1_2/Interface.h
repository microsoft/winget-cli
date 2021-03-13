// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_1/Interface.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_1::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        SQLite::rowid_t RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;

        // Version 1.2
        Utility::NormalizedName NormalizeName(std::string_view name, std::string_view publisher) const override;

    protected:
        std::unique_ptr<V1_0::SearchResultsTable> CreateSearchResultsTable(const SQLite::Connection& connection) const override;
        SearchResult SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const override;
        void PrepareForPackaging(SQLite::Connection& connection, bool vacuum) override;

        // The name normalization utility
        Utility::NameNormalizer m_normalizer;
    };
}
