// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/2_1/DeltaTables.h"

#include "Microsoft/Schema/2_0/PackagesTable.h"
#include "Microsoft/Schema/2_0/TagsTable.h"
#include "Microsoft/Schema/2_0/CommandsTable.h"
#include "Microsoft/Schema/2_0/PackageFamilyNameTable.h"
#include "Microsoft/Schema/2_0/ProductCodeTable.h"
#include "Microsoft/Schema/2_0/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/2_0/NormalizedPackagePublisherTable.h"
#include "Microsoft/Schema/2_0/UpgradeCodeTable.h"

#include <winget/SQLiteStatementBuilder.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    using namespace std::string_view_literals;

    namespace
    {
        constexpr std::string_view s_Delta_TablePrefix = "delta_"sv;
        constexpr std::string_view s_Delta_MapTableSuffix = "_map"sv;
        constexpr std::string_view s_Delta_ValueIndexSuffix = "_pkindex"sv;
        constexpr std::string_view s_Delta_IsRemovedColumn = "is_removed"sv;

        template <typename Table>
        ValueTableInfo MakeValueTableInfo()
        {
            return { Table::TableName(), Table::ValueName() };
        }
    }

    std::vector<ValueTableInfo> SystemReferenceTables()
    {
        return std::vector<ValueTableInfo>{
            MakeValueTableInfo<V2_0::PackageFamilyNameTable>(),
            MakeValueTableInfo<V2_0::ProductCodeTable>(),
            MakeValueTableInfo<V2_0::NormalizedPackageNameTable>(),
            MakeValueTableInfo<V2_0::NormalizedPackagePublisherTable>(),
            MakeValueTableInfo<V2_0::UpgradeCodeTable>(),
        };
    }

    std::vector<ValueTableInfo> OneToManyTables()
    {
        return std::vector<ValueTableInfo>{
            MakeValueTableInfo<V2_0::TagsTable>(),
            MakeValueTableInfo<V2_0::CommandsTable>(),
        };
    }

    std::string GetTableName(std::string_view baseTableName)
    {
        auto result = std::string{ s_Delta_TablePrefix };
        result.append(baseTableName);
        return result;
    }

    std::string GetMapTableName(std::string_view baseTableName)
    {
        auto result = GetTableName(baseTableName);
        result.append(s_Delta_MapTableSuffix);
        return result;
    }

    std::string_view IsRemovedColumnName()
    {
        return s_Delta_IsRemovedColumn;
    }

    bool IsDeltaDatabase(const SQLite::Connection& connection)
    {
        return SQLite::Builder::Schema::TableExists(connection, GetTableName(V2_0::PackagesTable::TableName()));
    }

    void CreateTables(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "delta_createtables_v2_1");

        // The packages table mirrors its 2.0 counterpart, with the addition of the removal flag.
        // Every column other than the identifier is nullable here, because a row that records a
        // removal carries no data beyond the identity of what was removed.
        {
            std::string tableName = GetTableName(V2_0::PackagesTable::TableName());

            StatementBuilder builder;
            builder.CreateTable(tableName).Columns({
                IntegerPrimaryKey(),
                ColumnBuilder(V2_0::PackagesTable::IdColumn::Name, Type::Text).NotNull(),
                ColumnBuilder(V2_0::PackagesTable::NameColumn::Name, Type::Text),
                ColumnBuilder(V2_0::PackagesTable::MonikerColumn::Name, Type::Text),
                ColumnBuilder(V2_0::PackagesTable::LatestVersionColumn::Name, Type::Text),
                ColumnBuilder(V2_0::PackagesTable::ARPMinVersionColumn::Name, Type::Text),
                ColumnBuilder(V2_0::PackagesTable::ARPMaxVersionColumn::Name, Type::Text),
                ColumnBuilder(V2_0::PackagesTable::HashColumn::Name, Type::Blob),
                ColumnBuilder(s_Delta_IsRemovedColumn, Type::Int64).NotNull()
                });
            builder.Execute(connection);
        }

        // The system reference tables hold the value itself, so the delta only adds the removal flag.
        for (const auto& table : SystemReferenceTables())
        {
            std::string tableName = GetTableName(table.TableName);

            StatementBuilder builder;
            builder.CreateTable(tableName).Columns({
                ColumnBuilder(table.ValueName, Type::Text).NotNull(),
                ColumnBuilder(V2_0::details::SystemReferenceStringTableGetPrimaryColumnName(), Type::RowId).NotNull(),
                ColumnBuilder(s_Delta_IsRemovedColumn, Type::Int64).NotNull(),
                PrimaryKeyBuilder({ table.ValueName, V2_0::details::SystemReferenceStringTableGetPrimaryColumnName() })
                }).WithoutRowID();
            builder.Execute(connection);
        }

        for (const auto& table : OneToManyTables())
        {
            // The data table holds only values that the baseline does not already have. There is no
            // removal flag because a value is only unreferenced once every map entry naming it is
            // removed, which the map table already records.
            {
                std::string tableName = GetTableName(table.TableName);

                StatementBuilder builder;
                builder.CreateTable(tableName).Columns({
                    IntegerPrimaryKey(),
                    ColumnBuilder(table.ValueName, Type::Text).NotNull()
                    });
                builder.Execute(connection);

                // Generation looks values up by string to reuse an already allocated rowid.
                StatementBuilder indexBuilder;
                indexBuilder.CreateUniqueIndex({ tableName, s_Delta_ValueIndexSuffix }).
                    On(tableName).Columns(table.ValueName);
                indexBuilder.Execute(connection);
            }

            {
                std::string mapTableName = GetMapTableName(table.TableName);

                StatementBuilder builder;
                builder.CreateTable(mapTableName).Columns({
                    ColumnBuilder(table.ValueName, Type::Int64).NotNull(),
                    ColumnBuilder(V2_0::details::OneToManyTableGetManifestColumnName(), Type::Int64).NotNull(),
                    ColumnBuilder(s_Delta_IsRemovedColumn, Type::Int64).NotNull(),
                    PrimaryKeyBuilder({ table.ValueName, V2_0::details::OneToManyTableGetManifestColumnName() })
                    }).WithoutRowID();
                builder.Execute(connection);
            }
        }

        savepoint.Commit();
    }

    void PrepareTablesForPackaging(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        // Every index here exists only to serve generation: those on the one to many data tables
        // let generation find the rowid it already allocated for a value.
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "delta_prepare_tables_v2_1");

            for (const auto& table : OneToManyTables())
            {
                StatementBuilder builder;
                auto tableName = GetTableName(table.TableName);
                builder.DropIndex({ tableName, s_Delta_ValueIndexSuffix });
                builder.Execute(connection);
            }

            savepoint.Commit();
        }

        StatementBuilder vacuumBuilder;
        vacuumBuilder.Vacuum();
        vacuumBuilder.Execute(connection);
    }
}
