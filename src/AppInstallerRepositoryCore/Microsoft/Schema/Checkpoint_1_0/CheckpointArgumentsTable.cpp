// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "CheckpointArgumentsTable.h"
#include "SQLiteStatementBuilder.h"
#include "Microsoft/Schema/ICheckpointIndex.h"

namespace AppInstaller::Repository::Microsoft::Schema::Checkpoint_V1_0
{
    using namespace SQLite;
    using namespace std::string_view_literals;
    static constexpr std::string_view s_CheckpointArgumentsTable_Table_Name = "CheckpointArguments"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_ContextId_Column = "ContextId"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Query_Column = "Query"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_MultiQuery_Column = "MultiQuery"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Manifest_Column = "Manifest"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Id_Column = "Id"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Moniker_Column = "Moniker"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Version_Column = "Version"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Channel_Column = "Channel"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Source_Column = "Source"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_InstallScope_Column = "InstallScope"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_InstallArchitecture_Column = "InstallArchitecture"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Exact_Column = "Exact"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Interactive_Column = "Interactive"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Silent_Column = "Silent"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Locale_Column = "Locale"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Log_Column = "Log"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_CustomSwitches_Column = "CustomSwitches"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Override_Column = "Override"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_InstallLocation_Column = "InstallLocation"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_HashOverride_Column = "HashOverride"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_SkipDependencies_Column = "SkipDependencies"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_IgnoreLocalArchiveMalwareScan_Column = "IgnoreLocalArchiveMalwareScan"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_DependencySource_Column = "DependencySource"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_AcceptPackageAgreements_Column = "AcceptPackageAgreements"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_NoUpgrade_Column = "NoUpgrade"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_CustomHeader_Column = "CustomHeader"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_AcceptSourceAgreements_Column = "AcceptSourceAgreements"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Rename_Column = "Rename"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_UninstallPrevious_Column = "UninstallPrevious"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Force_Column = "Force"sv;

    std::string_view CheckpointArgumentsTable::TableName()
    {
        return s_CheckpointArgumentsTable_Table_Name;
    }

    void CheckpointArgumentsTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createCheckpointArgumentsTable_v1_0");

        StatementBuilder createTableBuilder;

        createTableBuilder.CreateTable(s_CheckpointArgumentsTable_Table_Name).BeginColumns();

        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_ContextId_Column, Type::Int).PrimaryKey().NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Query_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_MultiQuery_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Manifest_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Id_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Moniker_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Version_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Channel_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Source_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_InstallScope_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_InstallArchitecture_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Exact_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Interactive_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Silent_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Locale_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Log_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_CustomSwitches_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Override_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_InstallLocation_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_SkipDependencies_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_IgnoreLocalArchiveMalwareScan_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_AcceptPackageAgreements_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_NoUpgrade_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_CustomHeader_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_AcceptSourceAgreements_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Rename_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_UninstallPrevious_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Force_Column, Type::Bool));

        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);
        savepoint.Commit();
    }

    bool CheckpointArgumentsTable::ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointArgumentsTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) != 0);
    }

    void CheckpointArgumentsTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointArgumentsTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
    }

    bool CheckpointArgumentsTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointArgumentsTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }

    std::optional<SQLite::rowid_t> CheckpointArgumentsTable::SelectByArgumentType(const SQLite::Connection& connection, int type)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointArgumentsTable_Table_Name).Where("id");
        builder.Equals(type);

        SQLite::Statement select = builder.Prepare(connection);

        if (select.Step())
        {
            return select.GetColumn<SQLite::rowid_t>(0);
        }
        else
        {
            return {};
        }
    }

    bool CheckpointArgumentsTable::UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, std::string_view value)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Update(s_CheckpointArgumentsTable_Table_Name).Set()
            .Column(name).Equals(value)
            .Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId);

        builder.Execute(connection);
        return connection.GetChanges() != 0;
    }

    bool CheckpointArgumentsTable::UpdateArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name, bool value)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Update(s_CheckpointArgumentsTable_Table_Name).Set()
            .Column(name).Equals(value)
            .Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId);

        builder.Execute(connection);
        return connection.GetChanges() != 0;
    }

    SQLite::rowid_t CheckpointArgumentsTable::AddContext(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointArgumentsTable_Table_Name)
            .Columns({ s_CheckpointArgumentsTable_ContextId_Column })
            .Values(contextId);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    void CheckpointArgumentsTable::RemoveContext(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointArgumentsTable_Table_Name).Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId);
        builder.Execute(connection);
    }

    bool CheckpointArgumentsTable::ContainsArgument(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointArgumentsTable_Table_Name)
            .Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId).And(name).IsNotNull();

        SQLite::Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    std::string CheckpointArgumentsTable::GetStringArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        using namespace Builder;

        StatementBuilder builder;
        builder.Select(name).From(s_CheckpointArgumentsTable_Table_Name).
            Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId);

        Statement statement = builder.Prepare(connection);
        if (statement.Step())
        {
            return statement.GetColumn<std::string>(0);
        }
        else
        {
            return {};
        }
    }

    bool CheckpointArgumentsTable::GetBoolArgumentByContextId(SQLite::Connection& connection, int contextId, std::string_view name)
    {
        using namespace Builder;

        StatementBuilder builder;
        builder.Select(name).From(s_CheckpointArgumentsTable_Table_Name).
            Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId);

        Statement statement = builder.Prepare(connection);
        if (statement.Step())
        {
            return statement.GetColumn<bool>(0);
        }
        else
        {
            return {};
        }
    }

    // This probably doesn't work...
    std::vector<std::string> CheckpointArgumentsTable::GetAvailableArguments(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select().From(s_CheckpointArgumentsTable_Table_Name).Where(s_CheckpointArgumentsTable_ContextId_Column).Equals(contextId);

        SQLite::Statement select = builder.Prepare(connection);

        std::vector<std::string> availableColumnNames;

        if (select.Step())
        {
            const std::tuple row = select.GetRow();
            int size = static_cast<int>(std::tuple_size<decltype(row)>{});

            // Start at column index 1 to skip 'rowid'
            for (int i = 1; i < size; i++)
            {
                availableColumnNames.emplace_back(select.GetColumnName(i));
            }

            return availableColumnNames;
        }
        else
        {
            return {};
        }

    }
}