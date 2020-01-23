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
        void AddManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
        void UpdateManifest(const Manifest::Manifest& oldManifest, const std::filesystem::path& oldRelativePath, const Manifest::Manifest& newManifest, const std::filesystem::path& newRelativePath) override;
        void RemoveManifest(const Manifest::Manifest& manifest, const std::filesystem::path& relativePath) override;
    };
}
