// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/2_1/DeltaConsistency.h"
#include "Microsoft/Schema/2_1/DeltaTables.h"
#include "Microsoft/Schema/2_1/Interface.h"

#include "Microsoft/Schema/2_0/PackagesTable.h"
#include "Microsoft/Schema/2_0/OneToManyTableWithMap.h"
#include "Microsoft/Schema/2_0/SystemReferenceStringTable.h"

#include <winget/SQLiteStatementBuilder.h>
#include <winget/SQLiteMetadataTable.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    using namespace std::string_view_literals;
    using namespace SQLite::Builder;

    namespace
    {
        // Aliases that keep the two references to a table apart in the correlated subqueries below.
        constexpr std::string_view s_Delta_RowAlias = "r"sv;
        constexpr std::string_view s_Delta_PackagesAlias = "p"sv;
        constexpr std::string_view s_Delta_MapAlias = "m"sv;

        // Every property that a package version can carry. Listed exhaustively rather than derived,
        // since neither enum has a sentinel value. A property that a given schema does not store
        // simply reads as empty on both sides, so nothing is lost by asking for all of them.
        constexpr PackageVersionProperty s_AllPackageVersionProperties[] =
        {
            PackageVersionProperty::Id,
            PackageVersionProperty::Name,
            PackageVersionProperty::SourceIdentifier,
            PackageVersionProperty::SourceName,
            PackageVersionProperty::Version,
            PackageVersionProperty::Channel,
            PackageVersionProperty::RelativePath,
            PackageVersionProperty::ManifestSHA256Hash,
            PackageVersionProperty::Publisher,
            PackageVersionProperty::ArpMinVersion,
            PackageVersionProperty::ArpMaxVersion,
            PackageVersionProperty::Moniker,
        };

        constexpr PackageVersionMultiProperty s_AllPackageVersionMultiProperties[] =
        {
            PackageVersionMultiProperty::PackageFamilyName,
            PackageVersionMultiProperty::ProductCode,
            PackageVersionMultiProperty::UpgradeCode,
            PackageVersionMultiProperty::Name,
            PackageVersionMultiProperty::Publisher,
            PackageVersionMultiProperty::Locale,
            PackageVersionMultiProperty::Tag,
            PackageVersionMultiProperty::Command,
        };

        // Enumerates the whole of an index, keyed by package identifier.
        std::map<std::string, SQLite::rowid_t> GetAllPackages(const ISQLiteIndex& index, const SQLite::Connection& connection)
        {
            // An empty request matches everything.
            ISQLiteIndex::SearchResult searchResult = index.Search(connection, {});

            // Nothing could be concluded from a partial enumeration.
            THROW_HR_IF(E_UNEXPECTED, searchResult.Truncated);

            std::map<std::string, SQLite::rowid_t> result;

            for (const auto& match : searchResult.Matches)
            {
                auto id = index.GetPropertyByPrimaryId(connection, match.first, PackageVersionProperty::Id);
                THROW_HR_IF(E_UNEXPECTED, !id);
                result.emplace(std::move(id).value(), match.first);
            }

            return result;
        }

        bool TableExists(const SQLite::Connection& connection, std::string_view tableName)
        {
            StatementBuilder builder;
            builder.Select(RowCount).From(SQLite::Builder::Schema::MainTable).
                Where(SQLite::Builder::Schema::TypeColumn).Equals(SQLite::Builder::Schema::Type_Table).
                And(SQLite::Builder::Schema::NameColumn).Equals(tableName);

            SQLite::Statement statement = builder.Prepare(connection);
            THROW_HR_IF(E_UNEXPECTED, !statement.Step());
            return statement.GetColumn<int64_t>(0) != 0;
        }

        // Every table that generation creates. A delta missing one of them cannot be read at all,
        // so this is established before anything tries to query them.
        std::vector<std::string> AllTableNames()
        {
            std::vector<std::string> result;

            result.emplace_back(GetTableName(V2_0::PackagesTable::TableName()));

            for (const auto& table : SystemReferenceTables())
            {
                result.emplace_back(GetTableName(table.TableName));
            }

            for (const auto& table : OneToManyTables())
            {
                result.emplace_back(GetTableName(table.TableName));
                result.emplace_back(GetMapTableName(table.TableName));
            }

            return result;
        }

        bool CheckTablesExist(const SQLite::Connection& connection, bool log)
        {
            bool result = true;

            for (const auto& tableName : AllTableNames())
            {
                if (!TableExists(connection, tableName))
                {
                    result = false;

                    if (!log)
                    {
                        break;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] delta table [" << tableName << "] does not exist");
                }
            }

            return result;
        }

        // A delta is only meaningful alongside the baseline that it names, and the client has to be
        // able to find that baseline from the delta alone. Missing values leave it unusable.
        bool CheckMetadata(const SQLite::Connection& connection, bool log)
        {
            bool result = true;

            for (std::string_view name : {
                s_MetadataValueName_DeltaBaselineIdentifier,
                s_MetadataValueName_DeltaBaselineRelativeSourcePath,
                s_MetadataValueName_DeltaBaselinePackageVersion })
            {
                auto value = SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, name);

                if (!value || value->empty())
                {
                    result = false;

                    if (!log)
                    {
                        break;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] delta metadata value [" << name << "] is missing or empty");
                }
            }

            return result;
        }

        bool CheckColumnForEmbeddedNulls(const SQLite::Connection& connection, std::string_view tableName, std::string_view columnName, bool log)
        {
            bool result = true;

            StatementBuilder builder;
            builder.Select(columnName).From(tableName).WhereValueContainsEmbeddedNullCharacter(columnName);

            SQLite::Statement select = builder.Prepare(connection);

            while (select.Step())
            {
                result = false;

                if (!log)
                {
                    break;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << columnName << "] in table [" << tableName <<
                    "] contains an embedded null character and starts with [" << select.GetColumn<std::string>(0) << "]");
            }

            return result;
        }

        // A row that records a removal carries no data beyond the identity of what was removed.
        // Data there would mean the row is simultaneously claiming the package is gone and
        // describing it, and the merged views would honor the removal and discard the description.
        bool CheckRemovalsCarryNoData(const SQLite::Connection& connection, bool log)
        {
            std::string tableName = GetTableName(V2_0::PackagesTable::TableName());
            bool result = true;

            for (std::string_view columnName : {
                V2_0::PackagesTable::NameColumn::Name,
                V2_0::PackagesTable::MonikerColumn::Name,
                V2_0::PackagesTable::LatestVersionColumn::Name,
                V2_0::PackagesTable::ARPMinVersionColumn::Name,
                V2_0::PackagesTable::ARPMaxVersionColumn::Name,
                V2_0::PackagesTable::HashColumn::Name })
            {
                StatementBuilder builder;
                builder.Select(V2_0::PackagesTable::IdColumn::Name).From(tableName).
                    Where(IsRemovedColumnName()).NotEqualsLiteral(0).
                    And(columnName).IsNotNull();

                SQLite::Statement select = builder.Prepare(connection);

                while (select.Step())
                {
                    result = false;

                    if (!log)
                    {
                        return result;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] removed package [" << select.GetColumn<std::string>(0) <<
                        "] in table [" << tableName << "] still carries a value for [" << columnName << "]");
                }
            }

            return result;
        }

        // A package removal is recorded once, in the packages table, rather than as a tombstone on
        // each of the package's associations. Generation therefore never writes an association for
        // a package that it is also recording as removed.
        bool CheckAssociationsAvoidRemovedPackages(
            const SQLite::Connection& connection,
            const std::string& tableName,
            std::string_view valueColumn,
            std::string_view packageColumn,
            bool log)
        {
            bool result = true;

            StatementBuilder builder;
            builder.Select({ QualifiedColumn{ s_Delta_RowAlias, valueColumn }, QualifiedColumn{ s_Delta_RowAlias, packageColumn } }).
                From(tableName).As(s_Delta_RowAlias).
                Where().Exists().BeginParenthetical().
                    Select(SQLite::RowIDName).
                    From(GetTableName(V2_0::PackagesTable::TableName())).As(s_Delta_PackagesAlias).
                    Where(QualifiedColumn{ s_Delta_PackagesAlias, SQLite::RowIDName }).
                        Equals(QualifiedColumn{ s_Delta_RowAlias, packageColumn }).
                    And(QualifiedColumn{ s_Delta_PackagesAlias, IsRemovedColumnName() }).NotEqualsLiteral(0).
                    EndParenthetical();

            SQLite::Statement select = builder.Prepare(connection);

            while (select.Step())
            {
                result = false;

                if (!log)
                {
                    break;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] " << tableName << " associates a value with package [" <<
                    select.GetColumn<SQLite::rowid_t>(1) << "], which the delta records as removed");
            }

            return result;
        }

        // Generation only creates a value row when it is about to map that value to a package, so a
        // value that nothing refers to means the map entry that justified it was lost. The reverse
        // direction cannot be checked here: a map entry may legitimately name a baseline value.
        bool CheckValuesAreReferenced(const SQLite::Connection& connection, const ValueTableInfo& table, bool log)
        {
            bool result = true;
            std::string valueTableName = GetTableName(table.TableName);

            StatementBuilder builder;
            builder.Select({ QualifiedColumn{ s_Delta_RowAlias, SQLite::RowIDName }, QualifiedColumn{ s_Delta_RowAlias, table.ValueName } }).
                From(valueTableName).As(s_Delta_RowAlias).
                Where().NotExists().BeginParenthetical().
                    Select(table.ValueName).
                    From(GetMapTableName(table.TableName)).As(s_Delta_MapAlias).
                    Where(QualifiedColumn{ s_Delta_MapAlias, table.ValueName }).
                        Equals(QualifiedColumn{ s_Delta_RowAlias, SQLite::RowIDName }).
                    And(QualifiedColumn{ s_Delta_MapAlias, IsRemovedColumnName() }).EqualsLiteral(0).
                    EndParenthetical();

            SQLite::Statement select = builder.Prepare(connection);

            while (select.Step())
            {
                result = false;

                if (!log)
                {
                    break;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] value [" << select.GetColumn<std::string>(1) << "] in table [" <<
                    valueTableName << "] is not referenced by any map entry that adds it");
            }

            return result;
        }
    }

    bool CheckConsistency(const SQLite::Connection& connection, bool log)
    {
        // A missing table would make every check below throw rather than report, so nothing else
        // runs until the shape of the database is established.
        if (!CheckTablesExist(connection, log))
        {
            return false;
        }

        bool result = true;

#define AICLI_CHECK_CONSISTENCY(_check_) \
        if (result || log) \
        { \
            result = _check_ && result; \
        }

        AICLI_CHECK_CONSISTENCY(CheckMetadata(connection, log));

        std::string packagesTableName = GetTableName(V2_0::PackagesTable::TableName());

        for (std::string_view columnName : {
            V2_0::PackagesTable::IdColumn::Name,
            V2_0::PackagesTable::NameColumn::Name,
            V2_0::PackagesTable::MonikerColumn::Name,
            V2_0::PackagesTable::LatestVersionColumn::Name,
            V2_0::PackagesTable::ARPMinVersionColumn::Name,
            V2_0::PackagesTable::ARPMaxVersionColumn::Name })
        {
            AICLI_CHECK_CONSISTENCY(CheckColumnForEmbeddedNulls(connection, packagesTableName, columnName, log));
        }

        AICLI_CHECK_CONSISTENCY(CheckRemovalsCarryNoData(connection, log));

        for (const auto& table : SystemReferenceTables())
        {
            std::string tableName = GetTableName(table.TableName);

            AICLI_CHECK_CONSISTENCY(CheckColumnForEmbeddedNulls(connection, tableName, table.ValueName, log));
            AICLI_CHECK_CONSISTENCY(CheckAssociationsAvoidRemovedPackages(
                connection, tableName, table.ValueName, V2_0::details::SystemReferenceStringTableGetPrimaryColumnName(), log));
        }

        for (const auto& table : OneToManyTables())
        {
            AICLI_CHECK_CONSISTENCY(CheckColumnForEmbeddedNulls(connection, GetTableName(table.TableName), table.ValueName, log));
            AICLI_CHECK_CONSISTENCY(CheckValuesAreReferenced(connection, table, log));
            AICLI_CHECK_CONSISTENCY(CheckAssociationsAvoidRemovedPackages(
                connection, GetMapTableName(table.TableName), table.ValueName, V2_0::details::OneToManyTableGetManifestColumnName(), log));
        }

#undef AICLI_CHECK_CONSISTENCY

        return result;
    }

    bool CheckEquivalence(
        const ISQLiteIndex& first,
        const SQLite::Connection& firstConnection,
        const ISQLiteIndex& second,
        const SQLite::Connection& secondConnection,
        bool log)
    {
        AICLI_LOG(Repo, Info, << "Checking index equivalence...");

        std::map<std::string, SQLite::rowid_t> firstPackages = GetAllPackages(first, firstConnection);
        std::map<std::string, SQLite::rowid_t> secondPackages = GetAllPackages(second, secondConnection);

        bool result = true;

        for (const auto& [id, primaryId] : firstPackages)
        {
            auto itr = secondPackages.find(id);

            if (itr == secondPackages.end())
            {
                result = false;
                if (!log) { return result; }
                AICLI_LOG(Repo, Info, << "  [INVALID] package [" << id << "] is not present in the comparison index");
                continue;
            }

            // The merged views suppress a baseline row by rowid, and every association is expressed
            // in terms of it, so equal contents alone would hide a divergence that matters.
            if (itr->second != primaryId)
            {
                result = false;
                if (!log) { return result; }
                AICLI_LOG(Repo, Info, << "  [INVALID] package [" << id << "] has primary id [" << primaryId <<
                    "] but [" << itr->second << "] in the comparison index");
            }

            for (auto property : s_AllPackageVersionProperties)
            {
                auto firstValue = first.GetPropertyByPrimaryId(firstConnection, primaryId, property);
                auto secondValue = second.GetPropertyByPrimaryId(secondConnection, itr->second, property);

                if (firstValue != secondValue)
                {
                    result = false;
                    if (!log) { return result; }
                    AICLI_LOG(Repo, Info, << "  [INVALID] package [" << id << "] property [" << static_cast<int>(property) <<
                        "] is [" << firstValue.value_or(std::string{}) << "] but [" << secondValue.value_or(std::string{}) <<
                        "] in the comparison index");
                }
            }

            for (auto property : s_AllPackageVersionMultiProperties)
            {
                auto firstValues = first.GetMultiPropertyByPrimaryId(firstConnection, primaryId, property);
                auto secondValues = second.GetMultiPropertyByPrimaryId(secondConnection, itr->second, property);

                // The order of these values is not part of what either index promises.
                std::sort(firstValues.begin(), firstValues.end());
                std::sort(secondValues.begin(), secondValues.end());

                if (firstValues != secondValues)
                {
                    result = false;
                    if (!log) { return result; }
                    AICLI_LOG(Repo, Info, << "  [INVALID] package [" << id << "] multi-property [" << static_cast<int>(property) <<
                        "] does not match the comparison index");
                }
            }
        }

        for (const auto& package : secondPackages)
        {
            if (firstPackages.find(package.first) == firstPackages.end())
            {
                result = false;
                if (!log) { return result; }
                AICLI_LOG(Repo, Info, << "  [INVALID] package [" << package.first << "] is present only in the comparison index");
            }
        }

        AICLI_LOG(Repo, Info, << "...indexes *WERE" << (result ? "*" : " NOT*") << " equivalent.");

        return result;
    }
}
