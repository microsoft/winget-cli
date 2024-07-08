// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_7/Interface.h"
#include <winget/SQLiteMetadataTable.h>
#include "Microsoft/Schema/1_0/CommandsTable.h"
#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_1/PackageFamilyNameTable.h"
#include "Microsoft/Schema/1_2/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/1_2/NormalizedPackagePublisherTable.h"
#include "Microsoft/Schema/1_6/UpgradeCodeTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_7
{
    namespace
    {
        bool ShouldFoldPropertyLookup(const SQLite::Connection& connection)
        {
            // Get the metadata indicator that we folded these multi properties.
            // If it contains the value for folding these properties in the 1.7 manner, also fold the incoming manifest
            // to the same value that it would have been folded to so that all manifest entries will have all of these properties.
            std::optional<std::string> mapDataFolded = SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, s_MetadataValueName_MapDataFolded);

            if (mapDataFolded)
            {
                std::vector<std::string> foldedSplit = Utility::Split(mapDataFolded.value(), s_MetadataValue_MapDataFolded_Separator);

                for (const std::string& splitValue : foldedSplit)
                {
                    if (splitValue == Interface::MapDataFolded_VersionSpecifier)
                    {
                        return true;
                    }
                }
            }

            return false;
        }
    }

    Interface::Interface(Utility::NormalizationVersion normVersion) : V1_6::Interface(normVersion)
    {
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 1, 7 };
    }

    std::vector<std::string> Interface::GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const
    {
        if (property == PackageVersionMultiProperty::PackageFamilyName ||
            property == PackageVersionMultiProperty::Name ||
            property == PackageVersionMultiProperty::Publisher ||
            property == PackageVersionMultiProperty::UpgradeCode)
        {
            if (ShouldFoldPropertyLookup(connection))
            {
                std::optional<SQLite::rowid_t> maximumManifestId = V1_0::OneToManyTableGetMapDataFoldingManifestTargetId(connection, primaryId);
                if (maximumManifestId)
                {
                    primaryId = maximumManifestId.value();
                }
            }
        }

        return V1_6::Interface::GetMultiPropertyByPrimaryId(connection, primaryId, property);
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_7");

        // Remove data that is not particularly interesting per-manifest
        std::array<SQLite::Statement, 6> dataRemovalStatements{
            V1_0::CommandsTable::PrepareMapDataFoldingStatement(connection),
            V1_0::TagsTable::PrepareMapDataFoldingStatement(connection),
            V1_1::PackageFamilyNameTable::PrepareMapDataFoldingStatement(connection),
            V1_2::NormalizedPackageNameTable::PrepareMapDataFoldingStatement(connection),
            V1_2::NormalizedPackagePublisherTable::PrepareMapDataFoldingStatement(connection),
            V1_6::UpgradeCodeTable::PrepareMapDataFoldingStatement(connection),
        };

        std::vector<SQLite::rowid_t> allIdentifiers = V1_0::IdTable::GetAllRowIds(connection);

        for (SQLite::rowid_t id : allIdentifiers)
        {
            for (SQLite::Statement& statement : dataRemovalStatements)
            {
                statement.Reset();
                statement.Bind(1, id);

                statement.Execute();
            }
        }

        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_MapDataFolded, MapDataFolded_VersionSpecifier);

        V1_6::Interface::PrepareForPackaging(connection, false);

        savepoint.Commit();

        if (vacuum)
        {
            Vacuum(connection);
        }
    }

    V1_0::OneToManyTableSchema Interface::GetOneToManyTableSchema() const
    {
        return V1_0::OneToManyTableSchema::Version_1_7;
    }
}
