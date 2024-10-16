// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_4/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_5
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_4::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        SQLite::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection, CreateOptions options) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath) override;
        void RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId) override;
        bool CheckConsistency(const SQLite::Connection& connection, bool log) const override;

    protected:

        bool NotNeeded(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id) const override;

        // Gets a property already knowing that the manifest id is valid.
        std::optional<std::string> GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const override;

    private:
        // Gets the ARP version ranges for the given package identifier.
        std::vector<Utility::VersionRange> GetArpVersionRanges(const SQLite::Connection& connection, SQLite::rowid_t packageIdentifier) const;

        // Semantic check to validate all arp version ranges within the index
        bool ValidateArpVersionConsistency(const SQLite::Connection& connection, bool log) const;
    };
}
