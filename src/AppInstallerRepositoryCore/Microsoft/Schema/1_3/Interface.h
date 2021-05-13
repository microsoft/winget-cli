// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_2/Interface.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_3
{
    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_2::Interface
    {
        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        Schema::Version GetVersion() const override;
        void CreateTables(SQLite::Connection& connection) override;
        SQLite::rowid_t AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        std::pair<bool, SQLite::rowid_t> UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;

    protected:
        // Gets a property already knowing that the manifest id is valid.
        std::optional<std::string> GetPropertyByManifestIdInternal(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const override;
    };
}
