// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/2_1/DeltaViews.h"
#include "Microsoft/Schema/2_1/DeltaTables.h"
#include "Microsoft/Schema/2_1/Interface.h"

#include "Microsoft/Schema/2_0/PackagesTable.h"
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"
#include "Microsoft/Schema/2_0/SystemReferenceStringTable.h"

#include <winget/SQLiteStatementBuilder.h>
#include <winget/SQLiteMetadataTable.h>
#include <winget/SQLiteVersion.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    using namespace std::string_view_literals;
    using namespace SQLite::Builder;

    namespace
    {
        // The schema name that the baseline database is attached under.
        constexpr std::string_view s_Delta_BaselineSchema = "baseline"sv;

        // Aliases for the two sides of a merge. The correlated subqueries below name the same
        // table on both sides, so the aliases are what keep the two references apart.
        constexpr std::string_view s_Delta_BaselineAlias = "b"sv;
        constexpr std::string_view s_Delta_DeltaAlias = "d"sv;
        constexpr std::string_view s_Delta_PackagesAlias = "p"sv;

        // Appends a test that the package owning the current baseline row still exists.
        //
        // When a package is removed, the delta records that fact once, in its packages table; it
        // does not write a removal row for each of the package's associations. This is therefore
        // the only thing standing between a removed package and its associations continuing to
        // appear in the merged data.
        void AppendPackageNotRemoved(StatementBuilder& builder, std::string_view packageColumn)
        {
            builder.NotExists().BeginParenthetical().
                Select(SQLite::RowIDName).
                From(GetTableName(V2_0::PackagesTable::TableName())).As(s_Delta_PackagesAlias).
                Where(QualifiedColumn{ s_Delta_PackagesAlias, SQLite::RowIDName }).
                    Equals(QualifiedColumn{ s_Delta_BaselineAlias, packageColumn }).
                And(QualifiedColumn{ s_Delta_PackagesAlias, IsRemovedColumnName() }).NotEqualsLiteral(0).
                EndParenthetical();
        }

        // Creates the view that merges the packages themselves.
        //
        // The delta holds a row for every package that changed and every package that was removed,
        // both keyed by the rowid that the package also has in the baseline. A baseline package is
        // therefore superseded whenever the delta mentions its rowid at all: the delta row replaces
        // it when it changed, and stands for its absence when it was removed.
        void CreatePackagesView(SQLite::Connection& connection)
        {
            std::string deltaPackages = GetTableName(V2_0::PackagesTable::TableName());

            StatementBuilder builder;
            builder.CreateTempView(V2_0::PackagesTable::TableName()).
                Select({
                    SQLite::RowIDName,
                    V2_0::PackagesTable::IdColumn::Name,
                    V2_0::PackagesTable::NameColumn::Name,
                    V2_0::PackagesTable::MonikerColumn::Name,
                    V2_0::PackagesTable::LatestVersionColumn::Name,
                    V2_0::PackagesTable::ARPMinVersionColumn::Name,
                    V2_0::PackagesTable::ARPMaxVersionColumn::Name,
                    V2_0::PackagesTable::HashColumn::Name }).
                From(deltaPackages).
                Where(IsRemovedColumnName()).EqualsLiteral(0).
                UnionAll().
                Select({
                    QualifiedColumn{ s_Delta_BaselineAlias, SQLite::RowIDName },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::IdColumn::Name },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::NameColumn::Name },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::MonikerColumn::Name },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::LatestVersionColumn::Name },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::ARPMinVersionColumn::Name },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::ARPMaxVersionColumn::Name },
                    QualifiedColumn{ s_Delta_BaselineAlias, V2_0::PackagesTable::HashColumn::Name } }).
                From(QualifiedTable{ s_Delta_BaselineSchema, V2_0::PackagesTable::TableName() }).As(s_Delta_BaselineAlias).
                Where().NotExists().BeginParenthetical().
                    Select(SQLite::RowIDName).From(deltaPackages).As(s_Delta_DeltaAlias).
                    Where(QualifiedColumn{ s_Delta_DeltaAlias, SQLite::RowIDName }).
                        Equals(QualifiedColumn{ s_Delta_BaselineAlias, SQLite::RowIDName }).
                    EndParenthetical();

            builder.Execute(connection);
        }

        // Creates the view that merges a table associating values with packages. This covers both
        // the system reference tables, which hold the value inline, and the one to many map tables,
        // which hold a reference to it; the two have the same shape as far as merging is concerned.
        //
        // The delta records only the associations that changed, rather than the full current set
        // for a changed package. A baseline association therefore survives unless the delta names
        // that exact pair, or the package it belongs to has gone away entirely.
        void CreateAssociationView(
            SQLite::Connection& connection,
            std::string_view viewName,
            const std::string& deltaTableName,
            std::string_view valueColumn,
            std::string_view packageColumn)
        {
            StatementBuilder builder;
            builder.CreateTempView(viewName).
                Select({ valueColumn, packageColumn }).
                From(deltaTableName).
                Where(IsRemovedColumnName()).EqualsLiteral(0).
                UnionAll().
                Select({
                    QualifiedColumn{ s_Delta_BaselineAlias, valueColumn },
                    QualifiedColumn{ s_Delta_BaselineAlias, packageColumn } }).
                From(QualifiedTable{ s_Delta_BaselineSchema, viewName }).As(s_Delta_BaselineAlias).
                Where().NotExists().BeginParenthetical().
                    Select(packageColumn).From(deltaTableName).As(s_Delta_DeltaAlias).
                    Where(QualifiedColumn{ s_Delta_DeltaAlias, valueColumn }).
                        Equals(QualifiedColumn{ s_Delta_BaselineAlias, valueColumn }).
                    And(QualifiedColumn{ s_Delta_DeltaAlias, packageColumn }).
                        Equals(QualifiedColumn{ s_Delta_BaselineAlias, packageColumn }).
                    EndParenthetical().
                And();

            AppendPackageNotRemoved(builder, packageColumn);

            builder.Execute(connection);
        }

        // Creates the view that merges a one to many data table.
        //
        // Nothing is ever suppressed here. The delta only contains values that the baseline has
        // never held, and it numbers them above the baseline's highest rowid, so the two sets are
        // disjoint. A value that no package refers to any more is left in place; the map table
        // governs what is visible, and an unreferenced value simply never appears.
        void CreateValueView(SQLite::Connection& connection, std::string_view tableName, std::string_view valueColumn)
        {
            StatementBuilder builder;
            builder.CreateTempView(tableName).
                Select({ SQLite::RowIDName, valueColumn }).
                From(GetTableName(tableName)).
                UnionAll().
                Select({ SQLite::RowIDName, valueColumn }).
                From(QualifiedTable{ s_Delta_BaselineSchema, tableName });

            builder.Execute(connection);
        }

        // Verifies that the attached baseline is the one that the delta was generated against.
        void ValidateAttachedBaseline(const SQLite::Connection& connection)
        {
            std::optional<std::string> expected =
                SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, s_MetadataValueName_DeltaBaselineIdentifier);

            std::optional<std::string> actual =
                SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, s_MetadataValueName_BaselineIdentifier, s_Delta_BaselineSchema);

            if (!expected || !actual || expected.value() != actual.value())
            {
                AICLI_LOG(Repo, Error, << "Delta expects baseline [" << expected.value_or("<none>") <<
                    "] but was given [" << actual.value_or("<none>") << "]");
                THROW_HR(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
            }

            // Ensure that the baseline also has the same schema version
            auto baselineVersion = SQLite::Version::GetSchemaVersion(connection, s_Delta_BaselineSchema);
            auto deltaVersion = SQLite::Version::GetSchemaVersion(connection);

            if (baselineVersion != deltaVersion)
            {
                AICLI_LOG(Repo, Error, << "Delta at version [" << deltaVersion << "] cannot use a baseline at version [" << baselineVersion << "]");
                THROW_HR(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
            }
        }
    }

    void SetupReadMode(SQLite::Connection& connection, const SQLite::DatabaseSpecifier& baseline)
    {
        AICLI_LOG(Repo, Info, << "Setting up delta read mode with baseline [" << baseline.Path() << "]");

        {
            StatementBuilder builder;
            builder.Attach(baseline, s_Delta_BaselineSchema);
            builder.Execute(connection);
        }

        try
        {
            ValidateAttachedBaseline(connection);
        }
        catch (...)
        {
            // Best effort attempt to restore the state
            try
            {
                StatementBuilder builder;
                builder.Detach(s_Delta_BaselineSchema);
                builder.Execute(connection);
            }
            CATCH_LOG();

            throw;
        }

        CreatePackagesView(connection);

        for (const auto& table : SystemReferenceTables())
        {
            CreateAssociationView(
                connection,
                table.TableName,
                GetTableName(table.TableName),
                table.ValueName,
                V2_0::details::SystemReferenceStringTableGetPrimaryColumnName());
        }

        for (const auto& table : OneToManyTables())
        {
            CreateValueView(connection, table.TableName, table.ValueName);

            CreateAssociationView(
                connection,
                V2_0::details::OneToManyTableGetMapTableName(table.TableName),
                GetMapTableName(table.TableName),
                table.ValueName,
                V2_0::details::OneToManyTableGetManifestColumnName());
        }
    }
}
