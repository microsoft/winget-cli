// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DependenciesTable.h"
#include <winget/SQLiteStatementBuilder.h>
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
    static constexpr std::string_view s_DependenciesTable_Index_Name = "dependencies_pkindex"sv;
    static constexpr std::string_view s_DependenciesTable_Manifest_Column_Name = "manifest"sv;
    static constexpr std::string_view s_DependenciesTable_MinVersion_Column_Name = "min_version"sv;
    static constexpr std::string_view s_DependenciesTable_PackageId_Column_Name = "package_id";

    namespace
    {
        struct DependencyTableRow 
        {
            SQLite::rowid_t m_packageRowId;
            SQLite::rowid_t m_manifestRowId;

            // Ideally this should be version row id, the version string is more needed than row id,
            // this prevents converting back and forth between version row id and version string.
            std::optional<Utility::NormalizedString> m_version; 

            bool operator <(const DependencyTableRow& rhs) const
            {
                auto lhsVersion = m_version.has_value() ? m_version.value() : "";
                auto rhsVersion = rhs.m_version.has_value() ? rhs.m_version.value() : "";
                return std::tie(m_packageRowId, m_manifestRowId, lhsVersion) < std::tie(rhs.m_packageRowId, rhs.m_manifestRowId, rhsVersion);
            }
        };

        void ThrowOnMissingPackageNodes(std::vector<Manifest::Dependency>& missingPackageNodes)
        {
            if (!missingPackageNodes.empty())
            {
                std::string missingPackages{ missingPackageNodes.begin()->Id()};
                std::for_each(
                    missingPackageNodes.begin() + 1,
                    missingPackageNodes.end(),
                    [&](auto& dep) { missingPackages.append(", " + dep.Id()); });
                THROW_HR_MSG(APPINSTALLER_CLI_ERROR_MISSING_PACKAGE, "Missing packages: %hs", missingPackages.c_str());
            }
        }

        std::set<DependencyTableRow> GetAndLinkDependencies(
            SQLite::Connection& connection,
            const Manifest::Manifest& manifest,
            SQLite::rowid_t manifestRowId,
            Manifest::DependencyType dependencyType)
        {
            std::set<DependencyTableRow>  dependencies;
            std::vector<Manifest::Dependency> missingPackageNodes;

            for (const auto& installer : manifest.Installers)
            {
                installer.Dependencies.ApplyToType(dependencyType, [&](Manifest::Dependency dependency)
                {
                    auto packageRowId = IdTable::SelectIdByValue(connection, dependency.Id(), true);
                    std::optional<Utility::NormalizedString> version;

                    if (!packageRowId.has_value())
                    {
                        missingPackageNodes.emplace_back(dependency);
                        return;
                    }

                    if (dependency.MinVersion.has_value())
                    {
                        version = dependency.MinVersion.value().ToString();
                    }

                    dependencies.emplace(DependencyTableRow{ packageRowId.value(), manifestRowId, version });
                });
            }

            ThrowOnMissingPackageNodes(missingPackageNodes);

            return dependencies;
        }

        bool RemoveDependenciesByRowIds(SQLite::Connection& connection, std::vector<DependencyTableRow> dependencyTableRows)
        {
            using namespace SQLite::Builder;
            bool tableUpdated = false;
            if (dependencyTableRows.empty())
            {
                return tableUpdated;
            }
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "remove_dependencies_by_rowid");

            SQLite::Builder::StatementBuilder builder;
            builder
                .DeleteFrom(s_DependenciesTable_Table_Name)
                .Where(s_DependenciesTable_PackageId_Column_Name).Equals(Unbound)
                .And(s_DependenciesTable_Manifest_Column_Name).Equals(Unbound);

            
            SQLite::Statement deleteStmt = builder.Prepare(connection);
            for (auto row : dependencyTableRows)
            {
                deleteStmt.Reset();
                deleteStmt.Bind(1, row.m_packageRowId);
                deleteStmt.Bind(2, row.m_manifestRowId);
                deleteStmt.Execute();
                tableUpdated = true;
            }

            savepoint.Commit();
            return tableUpdated;
        }

        bool InsertManifestDependencies(
            SQLite::Connection& connection,
            std::set<DependencyTableRow>& dependenciesTableRows)
        {
            using namespace SQLite::Builder;
            using namespace Schema::V1_0;
            bool tableUpdated = false;

            StatementBuilder insertBuilder;
            insertBuilder.InsertInto(s_DependenciesTable_Table_Name)
                .Columns({ s_DependenciesTable_Manifest_Column_Name, s_DependenciesTable_MinVersion_Column_Name, s_DependenciesTable_PackageId_Column_Name })
                .Values(Unbound, Unbound, Unbound);
            SQLite::Statement insert = insertBuilder.Prepare(connection);

            for (const auto& dep : dependenciesTableRows)
            {
                insert.Reset();
                insert.Bind(1, dep.m_manifestRowId);

                if (dep.m_version.has_value())
                {
                    insert.Bind(2, VersionTable::EnsureExists(connection, dep.m_version.value()));
                }
                else
                {
                    insert.Bind(2, nullptr);
                }
                
                insert.Bind(3, dep.m_packageRowId);

                insert.Execute();
                tableUpdated = true;
            }

            return tableUpdated;
        }
    }

    bool DependenciesTable::Exists(const SQLite::Connection& connection)
    {
        using namespace SQLite;

        Builder::StatementBuilder builder;
        builder.Select(Builder::RowCount).From(Builder::Schema::MainTable).
            Where(Builder::Schema::TypeColumn).Equals(Builder::Schema::Type_Table).And(Builder::Schema::NameColumn).Equals(s_DependenciesTable_Table_Name);

        Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    std::string_view DependenciesTable::TableName()
    {
        return s_DependenciesTable_Table_Name;
    }

    void DependenciesTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createDependencyTable_v1_4");
        constexpr std::string_view dependencyIndexByVersionId = "dependencies_version_id_index";
        constexpr std::string_view dependencyIndexByPackageId = "dependencies_package_id_index";

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(TableName()).BeginColumns();
        createTableBuilder.Column(IntegerPrimaryKey());

        std::array<DependenciesTableColumnInfo, 2> notNullableDependenciesColumns
        {
            DependenciesTableColumnInfo{ s_DependenciesTable_Manifest_Column_Name },
            DependenciesTableColumnInfo{ s_DependenciesTable_PackageId_Column_Name }
        };

        std::array<DependenciesTableColumnInfo, 1> nullableDependenciesColumns
        {
            DependenciesTableColumnInfo{ s_DependenciesTable_MinVersion_Column_Name }
        };

        // Add dependencies column tables not null columns.
        for (const DependenciesTableColumnInfo& value : notNullableDependenciesColumns)
        {
            createTableBuilder.Column(ColumnBuilder(value.Name, Type::RowId).NotNull());
        }

        // Add dependencies column tables null columns.
        for (const DependenciesTableColumnInfo& value : nullableDependenciesColumns)
        {
            createTableBuilder.Column(ColumnBuilder(value.Name, Type::RowId));
        }

        createTableBuilder.EndColumns();

        createTableBuilder.Execute(connection);

        // Primary key index by package rowid and manifest rowid.
        StatementBuilder createPKIndexBuilder;
        createPKIndexBuilder.CreateUniqueIndex(s_DependenciesTable_Index_Name).On(s_DependenciesTable_Table_Name).Columns({ s_DependenciesTable_Manifest_Column_Name, s_DependenciesTable_PackageId_Column_Name });
        createPKIndexBuilder.Execute(connection);

        // Index of dependency by Manifest id. 
        StatementBuilder createIndexByManifestIdBuilder;
        createIndexByManifestIdBuilder.CreateIndex(dependencyIndexByVersionId).On(s_DependenciesTable_Table_Name).Columns({ s_DependenciesTable_MinVersion_Column_Name });
        createIndexByManifestIdBuilder.Execute(connection);

        // Index of dependency by package id.
        StatementBuilder createIndexByPackageIdBuilder;
        createIndexByPackageIdBuilder.CreateIndex(dependencyIndexByPackageId).On(s_DependenciesTable_Table_Name).Columns({ s_DependenciesTable_PackageId_Column_Name });
        createIndexByPackageIdBuilder.Execute(connection);

        savepoint.Commit();
    }

    void DependenciesTable::Drop(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder dropTableBuilder;
        dropTableBuilder.DropTableIfExists(s_DependenciesTable_Table_Name);

        dropTableBuilder.Execute(connection);
    }

    void DependenciesTable::AddDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId)
    {
        if (!Exists(connection))
        {
            return;
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "add_dependencies_v1_4");

        auto dependencies = GetAndLinkDependencies(connection, manifest, manifestRowId, Manifest::DependencyType::Package);
        if (!dependencies.size())
        {
            return;
        }

        InsertManifestDependencies(connection, dependencies);

        savepoint.Commit();
    }

    bool DependenciesTable::UpdateDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId)
    {
        if (!Exists(connection))
        {
            return false;
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "update_dependencies_v1_4");

        const auto dependencies = GetAndLinkDependencies(connection, manifest, manifestRowId, Manifest::DependencyType::Package);
        auto existingDependencies = GetDependenciesByManifestRowId(connection, manifestRowId);

        // Get dependencies to add.
        std::set<DependencyTableRow> toAddDependencies;
        std::copy_if(
            dependencies.begin(),
            dependencies.end(),
            std::inserter(toAddDependencies, toAddDependencies.begin()),
            [&](DependencyTableRow dep)
            {
                Utility::NormalizedString version = dep.m_version.has_value() ? dep.m_version.value() : "";
                return existingDependencies.find(std::make_pair(dep.m_packageRowId, version)) == existingDependencies.end();
            }
        );

        // Get dependencies to remove.
        std::vector<DependencyTableRow> toRemoveDependencies;
        std::for_each(
            existingDependencies.begin(),
            existingDependencies.end(),
            [&](std::pair<SQLite::rowid_t, Utility::NormalizedString> row)
            {
                if (dependencies.find(DependencyTableRow{row.first, manifestRowId, row.second}) == dependencies.end())
                {
                    toRemoveDependencies.emplace_back(DependencyTableRow{ row.first, manifestRowId });
                }
            }
        );

        bool tableUpdated = RemoveDependenciesByRowIds(connection, toRemoveDependencies);
        tableUpdated = InsertManifestDependencies(connection, toAddDependencies) || tableUpdated;
        savepoint.Commit();

        return tableUpdated;
    }

    void DependenciesTable::RemoveDependencies(SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
    {
        if (!Exists(connection))
        {
            return;
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "remove_dependencies_by_manifest_v1_4");

        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_DependenciesTable_Table_Name).Where(s_DependenciesTable_Manifest_Column_Name).Equals(manifestRowId);

        builder.Execute(connection);
        savepoint.Commit();
    }

    std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> DependenciesTable::GetDependentsById(const SQLite::Connection& connection, Manifest::string_t packageId)
    {
        constexpr std::string_view depTableAlias = "dep";
        constexpr std::string_view minVersionAlias = "minV";
        constexpr std::string_view packageIdAlias = "pId";


        StatementBuilder builder;
        // Find all manifest that depend on this package. Use outer join for joining version table as min_version may be NULL.
        // SELECT [dep].[manifest], [dep].[min_version], [pId].[id], [minV].[version] FROM [dependencies] AS [dep] 
        // LEFT OUTER JOIN [versions] AS [minV] ON [dep].[min_version] = [minV].[rowid] 
        // JOIN [ids] AS [pId] ON [pId].[rowid] = [dep].[package_id] 
        // WHERE [pId].[id] = ?
        builder.Select()
            .Column(QCol(depTableAlias, s_DependenciesTable_Manifest_Column_Name))
            .Column(QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name))
            .Column(QCol(packageIdAlias, IdTable::ValueName()))
            .Column(QCol(minVersionAlias, VersionTable::ValueName()))
            .From({ s_DependenciesTable_Table_Name }).As(depTableAlias)
            .LeftOuterJoin({ VersionTable::TableName() }).As(minVersionAlias)
            .On(QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name), QCol(minVersionAlias, SQLite::RowIDName))
            .Join({ IdTable::TableName() }).As(packageIdAlias)
            .On(QCol(packageIdAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_PackageId_Column_Name))
            .Where(QCol(packageIdAlias, IdTable::ValueName())).Equals(Unbound);

        SQLite::Statement stmt = builder.Prepare(connection);
        stmt.Bind(1, std::string{ packageId });

        std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> resultSet;

        while (stmt.Step())
        {
            Utility::NormalizedString version = "";
            if (!stmt.GetColumnIsNull(1))
            {
                // If min_version is not NULL, use the corresponding value from Version table.
                version = stmt.GetColumn<std::string>(3);
            }
            resultSet.emplace_back(
                std::make_pair(stmt.GetColumn<SQLite::rowid_t>(0), std::move(version)));
        }

        return resultSet;
    }

    std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> DependenciesTable::GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
    {
        SQLite::Builder::StatementBuilder builder;

        constexpr std::string_view depTableAlias = "dep";
        constexpr std::string_view minVersionAlias = "minV";

        std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> resultSet;

        // Use Outer join since min_version could have NULL value. 
        // SELECT [dep].[package_id], [dep].[min_version], [minV].[version] FROM [dependencies] AS [dep] 
        // LEFT OUTER JOIN [versions] AS [minV] ON [minV].[rowid] = [dep].[min_version]
        // WHERE [dep].[manifest] = ?
        builder.Select()
            .Column(QCol(depTableAlias, s_DependenciesTable_PackageId_Column_Name))
            .Column(QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name))
            .Column(QCol(minVersionAlias, VersionTable::ValueName()))
            .From({ s_DependenciesTable_Table_Name }).As(depTableAlias)
            .LeftOuterJoin({ VersionTable::TableName() }).As(minVersionAlias)
            .On(QCol(minVersionAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name))
            .Where(QCol(depTableAlias, s_DependenciesTable_Manifest_Column_Name)).Equals(Unbound);

        SQLite::Statement select = builder.Prepare(connection);

        select.Bind(1, manifestRowId);
        while (select.Step())
        {
            Utility::NormalizedString version = "";
            if (!select.GetColumnIsNull(1))
            {
                // If min_version is not NULL, use the corresponding value from Version table.
                version = select.GetColumn<std::string>(2);
            }
            resultSet.emplace(std::make_pair(select.GetColumn<SQLite::rowid_t>(0), std::move(version)));
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
    
    bool DependenciesTable::CheckConsistency(const SQLite::Connection& connection, bool log)
    {
        StatementBuilder builder;

        if (!Exists(connection))
        {
            return true;
        }

        builder.Select(QCol(s_DependenciesTable_Table_Name, SQLite::RowIDName))
            .From(s_DependenciesTable_Table_Name)
            .LeftOuterJoin(IdTable::TableName())
            .On(QCol(s_DependenciesTable_Table_Name, s_DependenciesTable_PackageId_Column_Name), QCol(IdTable::TableName(), SQLite::RowIDName))
            .LeftOuterJoin(ManifestTable::TableName())
            .On(QCol(s_DependenciesTable_Table_Name, s_DependenciesTable_Manifest_Column_Name), QCol(ManifestTable::TableName(), SQLite::RowIDName))
            .LeftOuterJoin(VersionTable::TableName())
            .On(QCol(s_DependenciesTable_Table_Name, s_DependenciesTable_MinVersion_Column_Name), QCol(VersionTable::TableName(), SQLite::RowIDName))
            .Where(QCol(ManifestTable::TableName(), SQLite::RowIDName)).IsNull()
            .Or(QCol(VersionTable::TableName(), SQLite::RowIDName)).IsNull().And(QCol(s_DependenciesTable_Table_Name, s_DependenciesTable_MinVersion_Column_Name)).IsNotNull()
            .Or(QCol(IdTable::TableName(), SQLite::RowIDName)).IsNull();

        SQLite::Statement select = builder.Prepare(connection);
        
        bool result = true;

        while (select.Step())
        {
            result = false;

            if (!log)
            {
                break;
            }

            AICLI_LOG(Repo, Info, << "  [INVALID] row in [" << s_DependenciesTable_Table_Name << "] rowid [" << select.GetColumn<SQLite::rowid_t>(0) << "]");
        }

        return result;
    }

    bool DependenciesTable::IsValueReferenced(const SQLite::Connection& connection, std::string_view tableName, SQLite::rowid_t valueRowId)
    {
        if (!Exists(connection))
        {
            return false;
        }

        StatementBuilder builder;

        if (tableName != V1_0::VersionTable::TableName())
        {
            return false;
        }

        std::array<std::string_view, 1> columns = { s_DependenciesTable_MinVersion_Column_Name };
        bool referenced = false;

        for(auto column: columns)
        {
            builder.Select(SQLite::RowIDName).From(s_DependenciesTable_Table_Name).Where(column).Equals(Unbound).Limit(1);

            SQLite::Statement select = builder.Prepare(connection);

            select.Bind(1, valueRowId);
            if (select.Step())
            {
                referenced = true;
                break;
            }
        }

        return referenced;
    }

    std::vector<SQLite::rowid_t> DependenciesTable::GetDependenciesMinVersionsRowIdByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
    {
        if (!Exists(connection))
        {
            return {};
        }

        StatementBuilder builder;

        std::vector<SQLite::rowid_t> result;

        // Find all versions for manifest row.
        // SELECT [min_version] FROM [dependencies]  
        // WHERE [manifest] = ?
        builder.Select()
            .Column(s_DependenciesTable_MinVersion_Column_Name)
            .From({ s_DependenciesTable_Table_Name })
            .Where(s_DependenciesTable_Manifest_Column_Name).Equals(Unbound);

        auto select = builder.Prepare(connection);

        select.Bind(1, manifestRowId);

        while (select.Step())
        {
            if (!select.GetColumnIsNull(0))
            {
                result.emplace_back(select.GetColumn<SQLite::rowid_t>(0));
            }
        }

        return result;
    }

    std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> DependenciesTable::GetAllDependenciesWithMinVersions(const SQLite::Connection& connection)
    {
        if (!Exists(connection))
        {
            return {};
        }

        std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> result;

        constexpr std::string_view depTableAlias = "dep";
        constexpr std::string_view minVersionAlias = "minV";

        StatementBuilder builder;

        // SELECT [dep].[package_id], [minV].[version] FROM [dependencies] AS [dep] 
        // JOIN [versions] AS [minV] ON [minV].[rowid] = [dep].[min_version]
        builder.Select()
            .Column(QCol(depTableAlias, s_DependenciesTable_PackageId_Column_Name))
            .Column(QCol(minVersionAlias, VersionTable::ValueName()))
            .From({ s_DependenciesTable_Table_Name }).As(depTableAlias)
            .Join({ VersionTable::TableName() }).As(minVersionAlias)
            .On(QCol(minVersionAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name));

        SQLite::Statement select = builder.Prepare(connection);

        while (select.Step())
        {
            Utility::NormalizedString version = "";
            if (!select.GetColumnIsNull(1))
            {
                version = select.GetColumn<std::string>(1);
            }

            if (!version.empty())
            {
                result.emplace_back(std::make_pair(select.GetColumn<SQLite::rowid_t>(0), version));
            }
        }

        return result;
    }
}
