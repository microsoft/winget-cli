// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DependenciesTable.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"
#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::V1_3
{
	using namespace std::string_view_literals;
	static constexpr std::string_view s_DependenciesTable_Table_Name = "dependencies"sv;
	static constexpr std::string_view s_DependenciesTable_Index_Name = "dependencies_index"sv;
	static constexpr std::string_view s_DependenciesTable_Manifest_Column_Name = "manifest"sv;
	static constexpr std::string_view s_DependenciesTable_MinVersion_Column_Name = "min_version"sv;
	static constexpr std::string_view s_DependenciesTable_PackageId_Column_Name = "package_id";

	std::string_view DependenciesTable::TableName()
	{
		return s_DependenciesTable_Table_Name;
	}

	std::map<std::string, SQLite::rowid_t> SelectIdsByValues(const SQLite::Connection& connection, std::string_view tableName, std::string_view columnName, const std::vector<Utility::NormalizedString>& values)
	{
		using namespace Schema::V1_0;
		using namespace SQLite::Builder;
		const std::string rowIdColumnName = "rowid";

		std::map<std::string, SQLite::rowid_t> result;

		StatementBuilder selectStatement;
		selectStatement.Select({ rowIdColumnName, columnName }).From(tableName).Where(columnName).In(values.size());

		SQLite::Statement select = selectStatement.Prepare(connection);
		int bindIndex = 1;
		for (const std::string& value : values)
		{
			select.Bind(bindIndex, value);
			bindIndex++;
		}

		while (select.Step())
		{
			result.emplace(select.GetColumn<std::string>(1), select.GetColumn<SQLite::rowid_t>(0));
		}

		return result;
	}

	void DependenciesTable::Create(SQLite::Connection& connection)
	{
		using namespace SQLite::Builder;
		SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createDependencyTable_v1_3");

		StatementBuilder createTableBuilder;
		createTableBuilder.CreateTable(TableName()).BeginColumns();
		createTableBuilder.Column(IntegerPrimaryKey());

		std::vector<DepedenciesTableColumnInfo> dependenciesColumns =
		{ 
			{ s_DependenciesTable_Manifest_Column_Name },
			{ s_DependenciesTable_MinVersion_Column_Name },
			{ s_DependenciesTable_PackageId_Column_Name }
		};

		for (const DepedenciesTableColumnInfo& value : dependenciesColumns)
		{
			createTableBuilder.Column(ColumnBuilder(value.Name, Type::Int64).NotNull());
		}

		createTableBuilder.EndColumns();

		createTableBuilder.Execute(connection);

		StatementBuilder createPKIndexBuilder;
		createPKIndexBuilder.CreateIndex(s_DependenciesTable_Index_Name).On(s_DependenciesTable_Table_Name).Columns({ s_DependenciesTable_Manifest_Column_Name });
		createPKIndexBuilder.Execute(connection);

		savepoint.Commit();
	}


	void DependenciesTable::AddDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId)
	{
		using namespace Schema::V1_0;
		using namespace SQLite::Builder;

		SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "add_dependencies");

		auto dependencies = GetDependencies(manifest, AppInstaller::Manifest::DependencyType::Package);
		if (!dependencies.size())
		{
			return;
		}

		InsertDependencies(connection, manifestRowId, dependencies);

		savepoint.Commit();
	}

	bool DependenciesTable::UpdateDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId)
	{
		using namespace Schema::V1_0;

		SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "update_dependencies");

		const auto dependencies = GetDependencies(manifest, AppInstaller::Manifest::DependencyType::Package);
		const std::set<Manifest::Dependency> dependenciesSet(dependencies.begin(), dependencies.end());
		if (!dependencies.size())
		{
			return false;
		}

		auto existingDependencies = GetDependenciesByManifestRowId(connection, manifestRowId);
		
		// Get dependencies to add.
		std::vector<Manifest::Dependency> toAddDependencies;
		std::copy_if(
			dependencies.begin(),
			dependencies.end(),
			std::back_inserter(toAddDependencies),
			[&](Manifest::Dependency dep) 
			{ 
				return existingDependencies.find(dep) == existingDependencies.end(); 
			}
		);

		// Get dependencies to remove.
		std::vector<SQLite::rowid_t> toRemoveDependencies;
		std::for_each(
			existingDependencies.begin(),
			existingDependencies.end(),
			[&](std::pair<Manifest::Dependency, SQLite::rowid_t> row)
			{ 
				if (dependenciesSet.find(row.first) == dependenciesSet.end())
					toRemoveDependencies.emplace_back(row.second);  
			}
		);
		
		InsertDependencies(connection, manifestRowId, toAddDependencies);
		RemoveDependencies(connection, toRemoveDependencies);
		savepoint.Commit();

		return true;
	}


	std::vector<AppInstaller::Manifest::Dependency> DependenciesTable::GetDependencies(
		const Manifest::Manifest& manifest, AppInstaller::Manifest::DependencyType dependencyType)
	{
		std::vector<AppInstaller::Manifest::Dependency>  dependencies;

		for (const auto& installer : manifest.Installers)
		{
			installer.Dependencies.ApplyToAll([&](AppInstaller::Manifest::Dependency dependency)
			{
					if (dependency.Type == dependencyType)
					{
						dependencies.emplace_back(dependency);
					}
			});
		}

		return dependencies;
	}

	void DependenciesTable::RemoveDependencies(SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
	{
		SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "remove_dependencies_by_manifest");
		SQLite::Builder::StatementBuilder builder;
		builder.DeleteFrom(s_DependenciesTable_Table_Name).Where(s_DependenciesTable_Manifest_Column_Name).Equals(manifestRowId);

		builder.Prepare(connection).Execute(true);
		savepoint.Commit();
	}

	std::map<Manifest::Dependency, SQLite::rowid_t> DependenciesTable::GetDependenciesByManifestRowId(SQLite::Connection& connection, SQLite::rowid_t manifestRowId)
	{
		using QCol = SQLite::Builder::QualifiedColumn;
		using namespace SQLite::Builder;
		using namespace Schema::V1_0;

		SQLite::Builder::StatementBuilder builder;

		constexpr std::string_view depTableAlias = "dep";
		constexpr std::string_view minVersionAlias = "minV";
		constexpr std::string_view packageIdAlias = "pId";

		std::map<Manifest::Dependency, SQLite::rowid_t> resultSet;

		// SELECT [dep].[rowid], [minV].[version], [pId].[id] FROM [dependencies] AS [dep] 
		// JOIN [versions] AS [minV] ON [minV].[rowid] = [dep].[min_version] 
		// JOIN [ids] AS [pId] ON [pId].[rowid] = [dep].[package_id] 
		// WHERE [dep].[manifest] = ?
		builder.Select().
			Column(QCol(depTableAlias, SQLite::RowIDName))
			.Column(QCol(minVersionAlias, VersionTable::ValueName()))
			.Column(QCol(packageIdAlias, IdTable::ValueName()))
			.From({ s_DependenciesTable_Table_Name }).As(depTableAlias)
			.Join({ VersionTable::TableName() }).As(minVersionAlias)
			.On(QCol(minVersionAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_MinVersion_Column_Name))
			.Join({ IdTable::TableName() }).As(packageIdAlias)
			.On(QCol(packageIdAlias, SQLite::RowIDName), QCol(depTableAlias, s_DependenciesTable_PackageId_Column_Name))
			.Where(QCol(depTableAlias, s_DependenciesTable_Manifest_Column_Name)).Equals(Unbound);

		SQLite::Statement select = builder.Prepare(connection);

		select.Bind(1, manifestRowId);
		while (select.Step())
		{
			resultSet.emplace(
				Manifest::Dependency(Manifest::DependencyType::Package, select.GetColumn<std::string>(2), select.GetColumn<std::string>(1)),
				select.GetColumn<SQLite::rowid_t>(0));
		}

		return resultSet;

	}

	void DependenciesTable::RemoveDependencies(SQLite::Connection& connection, std::vector<SQLite::rowid_t> dependencyRowId)
	{
		using namespace SQLite::Builder;
		SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ s_DependenciesTable_Table_Name } + "remove_dependencies_by_rowid");
		
		if (dependencyRowId.size() > 0)
		{
			SQLite::Builder::StatementBuilder builder;
			builder.DeleteFrom(s_DependenciesTable_Table_Name).Where(SQLite::RowIDName).In(dependencyRowId.size());

			SQLite::Statement deleteStmt = builder.Prepare(connection);
			int bindIndex = 1;
			for (SQLite::rowid_t rowId : dependencyRowId)
			{
				deleteStmt.Bind(bindIndex, rowId);
				bindIndex++;
			}

			deleteStmt.Execute(true);
		}

		savepoint.Commit();
	}

	void DependenciesTable::FindMissing(
		std::map<std::string, SQLite::rowid_t> resultSet, std::vector<Utility::NormalizedString>& searchSpace, std::vector<Utility::NormalizedString>& notFoundIds)
	{
		std::copy_if(
			searchSpace.begin(),
			searchSpace.end(),
			std::back_inserter(notFoundIds),
			[&](Utility::NormalizedString version) { return resultSet.find(version) == resultSet.end(); }
		);
	}

	void DependenciesTable::InsertDependencies(
		SQLite::Connection& connection,
		SQLite::rowid_t manifestRowId,
		std::vector<Manifest::Dependency>& dependencies)
	{
		using namespace SQLite::Builder;
		using namespace Schema::V1_0;

		std::vector<Utility::NormalizedString> minVersions;
		std::vector<Utility::NormalizedString> packageIds;
		std::for_each(
			dependencies.begin(),
			dependencies.end(),
			[&](AppInstaller::Manifest::Dependency dep) { minVersions.emplace_back(dep.MinVersion.value().ToString()); }
		);

		std::for_each(
			dependencies.begin(),
			dependencies.end(),
			[&](AppInstaller::Manifest::Dependency dep) { packageIds.emplace_back(dep.Id); }
		);

		ManifestTable::ExistsById(connection, manifestRowId);

		// SELECT FROM versions where id IN ("1.0.0", "1.1.0" )
		auto versionsResultSet = SelectIdsByValues(connection, VersionTable::TableName(), VersionTable::ValueName(), minVersions);
		std::vector<Utility::NormalizedString> notFoundVersion;
		FindMissing(versionsResultSet, minVersions, notFoundVersion);

		// SELECT FROM ids where id IN (Example.Id1, Example.Id2, .... )
		auto idResultSet = SelectIdsByValues(connection, IdTable::TableName(), IdTable::ValueName(), packageIds);
		std::vector<Utility::NormalizedString> notFoundIds;
		FindMissing(idResultSet, packageIds, notFoundIds);

		StatementBuilder insertBuilder;
		insertBuilder.InsertInto(s_DependenciesTable_Table_Name)
			.Columns({ s_DependenciesTable_Manifest_Column_Name, s_DependenciesTable_MinVersion_Column_Name, s_DependenciesTable_PackageId_Column_Name })
			.Values(Unbound, Unbound, Unbound);
		SQLite::Statement insert = insertBuilder.Prepare(connection);

		// TODO: use nsert with mulitple values, rather than run the query in a loop.
		for (const auto& dep : dependencies)
		{
			insert.Reset();
			insert.Bind(1, manifestRowId);
			insert.Bind(2, versionsResultSet[dep.MinVersion.value().ToString()]);
			insert.Bind(3, idResultSet[dep.Id]);

			insert.Execute(true);
		}
	}
}