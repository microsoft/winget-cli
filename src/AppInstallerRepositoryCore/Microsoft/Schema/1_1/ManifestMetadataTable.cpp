// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestMetadataTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    using namespace SQLite;

    static constexpr std::string_view s_ManifestMetadataTable_Table_Name = "manifest_metadata"sv;
    static constexpr std::string_view s_ManifestMetadataTable_PrimaryKeyIndex_Name = "manifest_metadata_pk"sv;
    static constexpr std::string_view s_ManifestMetadataTable_Manifest_Column = "manifest"sv;
    static constexpr std::string_view s_ManifestMetadataTable_Metadata_Column = "metadata"sv;
    static constexpr std::string_view s_ManifestMetadataTable_Value_Column = "value"sv;

    bool ManifestMetadataTable::Exists(const SQLite::Connection& connection)
    {
        Builder::StatementBuilder builder;
        builder.Select(Builder::RowCount).From(Builder::Schema::MainTable).
            Where(Builder::Schema::TypeColumn).Equals(Builder::Schema::Type_Table).And(Builder::Schema::NameColumn).Equals(s_ManifestMetadataTable_Table_Name);

        Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    void ManifestMetadataTable::Create(SQLite::Connection& connection)
    {
        using namespace Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createmanifestmetadata_v1_1");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_ManifestMetadataTable_Table_Name).Columns({
            ColumnBuilder(s_ManifestMetadataTable_Manifest_Column, Type::Int64).NotNull(),
            ColumnBuilder(s_ManifestMetadataTable_Metadata_Column, Type::Int64).NotNull(),
            ColumnBuilder(s_ManifestMetadataTable_Value_Column, Type::Text)
            });

        createTableBuilder.Execute(connection);

        StatementBuilder createPKIndexBuilder;
        createPKIndexBuilder.CreateUniqueIndex(s_ManifestMetadataTable_PrimaryKeyIndex_Name).On(s_ManifestMetadataTable_Table_Name).
            Columns({ s_ManifestMetadataTable_Manifest_Column, s_ManifestMetadataTable_Metadata_Column });
        createPKIndexBuilder.Execute(connection);

        savepoint.Commit();
    }

    ISQLiteIndex::MetadataResult ManifestMetadataTable::GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        using namespace Builder;

        StatementBuilder builder;
        builder.Select({ s_ManifestMetadataTable_Metadata_Column, s_ManifestMetadataTable_Value_Column }).From(s_ManifestMetadataTable_Table_Name).
            Where(s_ManifestMetadataTable_Manifest_Column).Equals(manifestId);

        Statement statement = builder.Prepare(connection);

        ISQLiteIndex::MetadataResult result;
        while (statement.Step())
        {
            result.emplace_back(std::make_pair(statement.GetColumn<PackageVersionMetadata>(0), statement.GetColumn<std::string>(1)));
        }

        return result;
    }

    void ManifestMetadataTable::SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value)
    {
        using namespace Builder;

        // First, we attempt to update an existing row. If not changes occurred, we then insert the new value.
        // UPSERT (aka ON CONFLICT) is not available to us, as it was only introduced in 3.24.0 (2018-06-04),
        // and we need to support Windows 10 (17763) which was released in 2017.
        StatementBuilder updateBuilder;
        updateBuilder.Update(s_ManifestMetadataTable_Table_Name).Set().Column(s_ManifestMetadataTable_Value_Column).Equals(value).
            Where(s_ManifestMetadataTable_Manifest_Column).Equals(manifestId).And(s_ManifestMetadataTable_Metadata_Column).Equals(metadata);

        updateBuilder.Execute(connection);

        // No changes means we need to insert the row
        if (connection.GetChanges() == 0)
        {
            StatementBuilder insertBuilder;
            insertBuilder.InsertInto(s_ManifestMetadataTable_Table_Name).
                Columns({ s_ManifestMetadataTable_Manifest_Column, s_ManifestMetadataTable_Metadata_Column, s_ManifestMetadataTable_Value_Column })
                .Values(manifestId, metadata, value);

            insertBuilder.Execute(connection);
        }
    }

    void ManifestMetadataTable::DeleteByManifestId(SQLite::Connection & connection, SQLite::rowid_t manifestId)
    {
        using namespace Builder;

        StatementBuilder builder;
        builder.DeleteFrom(s_ManifestMetadataTable_Table_Name).Where(s_ManifestMetadataTable_Manifest_Column).Equals(manifestId);
        builder.Execute(connection);
    }
}
