// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DependenciesTable.h"
#include "SQLiteStatementBuilder.h"
#include "winget\DependenciesGraph.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"
#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/Interface.h"
#include "Microsoft/Schema/1_0/ChannelTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_4
{
    using namespace AppInstaller;
    using namespace std::string_view_literals;
    using namespace SQLite::Builder;
    using namespace Schema::V1_0;
    using QCol = SQLite::Builder::QualifiedColumn;

    static constexpr std::string_view s_DependenciesTable_Table_Name = "dependencies"sv;
    static constexpr std::string_view s_DependenciesTable_Index_Name = "dependencies_index"sv;
    static constexpr std::string_view s_DependenciesTable_Manifest_Column_Name = "manifest"sv;
    static constexpr std::string_view s_DependenciesTable_MinVersion_Column_Name = "min_version"sv;
    static constexpr std::string_view s_DependenciesTable_PackageId_Column_Name = "package_id";

    namespace
    {
        struct DependencyTableRow 
        {
            SQLite::rowid_t m_packageRowId;
            SQLite::rowid_t m_manifestRowId;
            std::optional<SQLite::rowid_t> m_versionRowId;
        };

        std::set<Manifest::Dependency> GetDependencies(
            const Manifest::Manifest& manifest, Manifest::DependencyType dependencyType)
        {
            std::set<Manifest::Dependency>  dependencies;

            for (const auto& installer : manifest.Installers)
            {
                installer.Dependencies.ApplyToAll([&](Manifest::Dependency dependency)
                    {
                        if (dependency.Type == dependencyType)
                        {
                            dependencies.emplace(dependency);
                        }
                    });
            }

            return dependencies;
        }

        void RemoveDependenciesByRowIds(SQLite::Connection& connection, std::vector<SQLite::rowid_t> dependencyRowIds)
        {
            using namespace SQLite::Builder;
            if (dependencyRowIds.empty())
            {
                return;
            }
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "remove_dependencies_by_rowid");

            SQLite::Builder::StatementBuilder builder;
            builder.DeleteFrom(s_DependenciesTable_Table_Name).Where(SQLite::RowIDName).Equals(Unbound);

            SQLite::Statement deleteStmt = builder.Prepare(connection);
            for (SQLite::rowid_t rowId : dependencyRowIds)
            {
                deleteStmt.Reset();
                deleteStmt.Bind(1, rowId);
                deleteStmt.Execute(true);
            }

            savepoint.Commit();
        }

        void ThrowOnMissingPackageNodes(std::vector<Manifest::Dependency> missingPackageNodes)
        {
            if (!missingPackageNodes.empty())
            {
                std::string missingPackages{ missingPackageNodes.begin()->Id };
                std::for_each(
                    missingPackageNodes.begin() + 1,
                    missingPackageNodes.end(),
                    [&](auto& dep) { missingPackages.append(", " + dep.Id);  });
                THROW_HR_MSG(APPINSTALLER_CLI_ERROR_MISSING_PACKAGE, "Missing packages: %hs", missingPackages.c_str());
            }
        }

        void InsertManifestDependencies(
            SQLite::Connection& connection,
            SQLite::rowid_t manifestRowId,
            std::set<Manifest::Dependency>& dependencies)
        {
            using namespace SQLite::Builder;
            using namespace Schema::V1_0;

            std::vector<DependencyTableRow> dependenciesTableRows;
            std::vector<Manifest::Dependency> missingPackageNodes;
            std::map<Utility::Version, SQLite::rowid_t> versionsMap;
            std::map<Manifest::Manifest::string_t, SQLite::rowid_t> idsMap;
            std::vector<std::tuple<SQLite::rowid_t, SQLite::rowid_t, SQLite::rowid_t>> dependencyRows;

            for (const auto& dep : dependencies)
            {
                auto depMinVersion = dep.MinVersion;
                auto packageId = dep.Id;

                const auto packageRowId = IdTable::SelectIdByValue(connection, packageId);
                if (!packageRowId.has_value())
                {
                    missingPackageNodes.emplace_back(dep);
                    continue;
                }

                std::optional<SQLite::rowid_t> versionRowId;
                if (depMinVersion.has_value())
                {
                    versionRowId = VersionTable::EnsureExists(connection, dep.MinVersion.value().ToString());
                }

                dependenciesTableRows.emplace_back(DependencyTableRow{ packageRowId.value(), manifestRowId, versionRowId });
            }

            ThrowOnMissingPackageNodes(missingPackageNodes);

            StatementBuilder insertBuilder;
            insertBuilder.InsertInto(s_DependenciesTable_Table_Name)
                .Columns({ s_DependenciesTable_Manifest_Column_Name, s_DependenciesTable_MinVersion_Column_Name, s_DependenciesTable_PackageId_Column_Name })
                .Values(Unbound, Unbound, Unbound);
            SQLite::Statement insert = insertBuilder.Prepare(connection);

            for (const auto& dep : dependenciesTableRows)
            {
                insert.Reset();
                insert.Bind(1, dep.m_manifestRowId);
                insert.Bind(2, dep.m_versionRowId.has_value() ? std::to_string(dep.m_versionRowId.value()) : "");
                insert.Bind(3, dep.m_packageRowId);

                insert.Execute(true);
            }
        }

        std::optional<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetPackageRowIdAndVersion(SQLite::Connection& connection, const Manifest::Dependency& dependency)
        {
            auto packageId = IdTable::SelectIdByValue(connection, dependency.Id);

            if (packageId.has_value())
            {
                return std::make_pair(packageId.value(), dependency.MinVersion.value().ToString());
            }

            return {};
        }
    }

    std::string_view DependenciesTable::TableName()
    {
        return s_DependenciesTable_Table_Name;
    }

    void DependenciesTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createDependencyTable_v1_4");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(TableName()).BeginColumns();
        createTableBuilder.Column(IntegerPrimaryKey());

        std::vector<DependenciesTableColumnInfo> notNullabledependenciesColumns =
        {
            { s_DependenciesTable_Manifest_Column_Name },
            { s_DependenciesTable_MinVersion_Column_Name },
        };

        for (const DependenciesTableColumnInfo& value : notNullabledependenciesColumns)
        {
            createTableBuilder.Column(ColumnBuilder(value.Name, Type::RowId).NotNull());
        }

        std::vector<DependenciesTableColumnInfo> nullableDependenciesColumns =
        {
            { s_DependenciesTable_PackageId_Column_Name }
        };

        for (const DependenciesTableColumnInfo& value : nullableDependenciesColumns)
        {
            createTableBuilder.Column(ColumnBuilder(value.Name, Type::RowId));
        }

        createTableBuilder.EndColumns();

        createTableBuilder.Execute(connection);

        StatementBuilder createPKIndexBuilder;
        createPKIndexBuilder.CreateUniqueIndex(s_DependenciesTable_Index_Name).On(s_DependenciesTable_Table_Name).Columns({ s_DependenciesTable_Manifest_Column_Name, s_DependenciesTable_MinVersion_Column_Name, s_DependenciesTable_PackageId_Column_Name });
        createPKIndexBuilder.Execute(connection);

        savepoint.Commit();
    }


    void DependenciesTable::AddDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "add_dependencies_v1_4");

        auto dependencies = GetDependencies(manifest, Manifest::DependencyType::Package);
        if (!dependencies.size())
        {
            return;
        }

        InsertManifestDependencies(connection, manifestRowId, dependencies);

        savepoint.Commit();
    }

    bool DependenciesTable::UpdateDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "update_dependencies_v1_4");

        const auto dependencies = GetDependencies(manifest, Manifest::DependencyType::Package);
        auto existingDependencies = GetDependenciesByManifestRowId(connection, manifestRowId);

        // Get dependencies to add.
        std::set<Manifest::Dependency> toAddDependencies;
        std::copy_if(
            dependencies.begin(),
            dependencies.end(),
            std::inserter(toAddDependencies, toAddDependencies.begin()),
            [&](Manifest::Dependency dep)
            {
                auto depRow = GetPackageRowIdAndVersion(connection, dep);
                return existingDependencies.find(depRow.value()) == existingDependencies.end();
            }
        );

        // Get dependencies to remove.
        std::vector<SQLite::rowid_t> toRemoveDependencies;
        std::vector<Manifest::Dependency> missingPackageNodes;
        std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> dependenciesPackageIdAndVersion;
        std::for_each(
            dependencies.begin(),
            dependencies.end(),
            [&](auto& dep)
            {
                auto depRow = GetPackageRowIdAndVersion(connection, dep);
                if (!depRow.has_value())
                {
                    missingPackageNodes.emplace_back(dep);
                    return;
                }

                dependenciesPackageIdAndVersion.emplace(depRow.value());
            }
        );

        ThrowOnMissingPackageNodes(missingPackageNodes);

        std::for_each(
            existingDependencies.begin(),
            existingDependencies.end(),
            [&](std::pair<std::pair<SQLite::rowid_t, Utility::NormalizedString>, SQLite::rowid_t> row)
            {
                if (dependenciesPackageIdAndVersion.find(row.first) == dependenciesPackageIdAndVersion.end())
                {
                    toRemoveDependencies.emplace_back(row.second);
                }
            }
        );

        InsertManifestDependencies(connection, manifestRowId, toAddDependencies);
        RemoveDependenciesByRowIds(connection, toRemoveDependencies);
        savepoint.Commit();

        return true;
    }

    void DependenciesTable::RemoveDependencies(SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "remove_dependencies_by_manifest_v1_4");
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_DependenciesTable_Table_Name).Where(s_DependenciesTable_Manifest_Column_Name).Equals(manifestRowId);

        builder.Prepare(connection).Execute(true);
        savepoint.Commit();
    }

    std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> DependenciesTable::GetDependentsById(const SQLite::Connection& connection, Manifest::string_t packageId)
    {
        constexpr std::string_view depTableAlias = "dep";
        constexpr std::string_view minVersionAlias = "minV";
        constexpr std::string_view packageIdAlias = "pId";


        StatementBuilder builder;
        // Find all manifest that depend on this package.
        // SELECT [dep].[manifest], [pId].[id], [minV].[version] FROM [dependencies] AS [dep] 
        // JOIN [versions] AS [minV] ON [dep].[min_version] = [minV].[rowid] 
        // JOIN [ids] AS [pId] ON [pId].[rowid] = [dep].[package_id] 
        // WHERE [pId].[id] = ?
        builder.Select()
            .Column(QCol(depTableAlias, s_DependenciesTable_Manifest_Column_Name))
            .Column(QCol(packageIdAlias, IdTable::ValueName()))
            .Column(QCol(minVersionAlias, VersionTable::ValueName()))
            .From({ s_DependenciesTable_Table_Name }).As(depTableAlias)
            .Join({ VersionTable::TableName() }).As(minVersionAlias)
            .On(QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name), QCol(minVersionAlias, SQLite::RowIDName))
            .Join({ IdTable::TableName() }).As(packageIdAlias)
            .On(QCol(packageIdAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_PackageId_Column_Name))
            .Where(QCol(packageIdAlias, IdTable::ValueName())).Equals(Unbound);

        SQLite::Statement stmt = builder.Prepare(connection);
        stmt.Bind(1, std::string{ packageId });

        std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> resultSet;

        while (stmt.Step())
        {
            resultSet.emplace_back(
                std::make_pair(stmt.GetColumn<SQLite::rowid_t>(0), Utility::NormalizedString(stmt.GetColumn<std::string>(2))));
        }

        return resultSet;
    }

    std::map<std::pair<SQLite::rowid_t, Utility::NormalizedString>, SQLite::rowid_t> DependenciesTable::GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
    {
        SQLite::Builder::StatementBuilder builder;

        constexpr std::string_view depTableAlias = "dep";
        constexpr std::string_view minVersionAlias = "minV";

        std::map<std::pair<SQLite::rowid_t, Utility::NormalizedString>, SQLite::rowid_t> resultSet;

        // SELECT [dep].[rowid], [minV].[version], [dep].[id] FROM [dependencies] AS [dep] 
        // JOIN [versions] AS [minV] ON [minV].[rowid] = [dep].[min_version]
        // WHERE [dep].[manifest] = ?
        builder.Select().
            Column(QCol(depTableAlias, SQLite::RowIDName))
            .Column(QCol(minVersionAlias, VersionTable::ValueName()))
            .Column(QCol(depTableAlias, s_DependenciesTable_PackageId_Column_Name))
            .From({ s_DependenciesTable_Table_Name }).As(depTableAlias)
            .Join({ VersionTable::TableName() }).As(minVersionAlias)
            .On(QCol(minVersionAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name))
            .Where(QCol(depTableAlias, s_DependenciesTable_Manifest_Column_Name)).Equals(Unbound);

        SQLite::Statement select = builder.Prepare(connection);

        select.Bind(1, manifestRowId);
        while (select.Step())
        {
            resultSet.emplace(
                std::make_pair(select.GetColumn<SQLite::rowid_t>(2), Utility::NormalizedString(select.GetColumn<std::string>(1))),
                select.GetColumn<SQLite::rowid_t>(0));
        }

        return resultSet;

    }

    void DependenciesTable::PrepareForPackaging(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareForPacking_V1_4");

        StatementBuilder dropIndexBuilder;
        dropIndexBuilder.DropIndex({ s_DependenciesTable_Index_Name });
        dropIndexBuilder.Execute(connection);

        StatementBuilder dropTableBuilder;
        dropTableBuilder.DropTable({ s_DependenciesTable_Table_Name });
        dropTableBuilder.Execute(connection);

        savepoint.Commit();
    }
}