// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "Microsoft/Schema/1_6/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_7
{
    using namespace std::string_view_literals;

    // Version 1.7
    static constexpr std::string_view s_MetadataValueName_MapDataFolded = "mapDataFolded"sv;
    static constexpr char s_MetadataValue_MapDataFolded_Separator = ';';

    // Interface to this schema version exposed through ISQLiteIndex.
    struct Interface : public V1_6::Interface
    {
        static constexpr std::string_view MapDataFolded_VersionSpecifier = "1.7"sv;

        Interface(Utility::NormalizationVersion normVersion = Utility::NormalizationVersion::Initial);

        // Version 1.0
        SQLite::Version GetVersion() const override;
        std::vector<std::string> GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const override;

    protected:
        void PrepareForPackaging(SQLite::Connection& connection, bool vacuum) override;
        V1_0::OneToManyTableSchema GetOneToManyTableSchema() const override;
    };
}
