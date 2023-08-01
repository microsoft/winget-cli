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
    static constexpr std::string_view s_CheckpointArgumentsTable_ContextId_Column = "contextId"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_CommandName_Column = "commandName"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Query_Column = "query"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_MultiQuery_Column = "multiQuery"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Manifest_Column = "manifest"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Id_Column = "id"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Name_Column = "name"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Moniker_Column = "moniker"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Version_Column = "version"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Channel_Column = "channel"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Source_Column = "source"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_InstallScope_Column = "scope"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_InstallArchitecture_Column = "architecture"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Exact_Column = "exact"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Interactive_Column = "interactive"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Silent_Column = "silent"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Locale_Column = "locale"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Log_Column = "log"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Custom_Column = "custom"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Override_Column = "override"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_InstallLocation_Column = "location"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_HashOverride_Column = "ignore-security-hash"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_SkipDependencies_Column = "skip-dependencies"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_IgnoreLocalArchiveMalwareScan_Column = "ignore-local-archive-malware-scan"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_DependencySource_Column = "dependency-source"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_AcceptPackageAgreements_Column = "accept-package-agreements"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_NoUpgrade_Column = "no-upgrade"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_CustomHeader_Column = "header"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_AcceptSourceAgreements_Column = "accept-source-agreements"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Rename_Column = "rename"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_UninstallPrevious_Column = "uninstall-previous"sv;
    static constexpr std::string_view s_CheckpointArgumentsTable_Force_Column = "force"sv;

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
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_ContextId_Column, Type::Int).Unique().NotNull());
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_CommandName_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Query_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_MultiQuery_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Manifest_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Id_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Name_Column, Type::Text));
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
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Custom_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_Override_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_InstallLocation_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_HashOverride_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_SkipDependencies_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_IgnoreLocalArchiveMalwareScan_Column, Type::Bool));
        createTableBuilder.Column(ColumnBuilder(s_CheckpointArgumentsTable_DependencySource_Column, Type::Text));
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

    std::optional<SQLite::rowid_t> CheckpointArgumentsTable::SelectByContextId(const SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_CheckpointArgumentsTable_Table_Name).Where(s_CheckpointArgumentsTable_ContextId_Column);
        builder.Equals(contextId);

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

    SQLite::rowid_t CheckpointArgumentsTable::AddContext(SQLite::Connection& connection, int contextId)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_CheckpointArgumentsTable_Table_Name)
            .Columns({ s_CheckpointArgumentsTable_ContextId_Column })
            .Values(contextId);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

    void CheckpointArgumentsTable::RemoveContextById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_CheckpointArgumentsTable_Table_Name).Where(SQLite::RowIDName).Equals(id);
        builder.Execute(connection);
    }

    bool CheckpointArgumentsTable::UpdateArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name, std::string_view value)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Update(s_CheckpointArgumentsTable_Table_Name).Set()
            .Column(name).Equals(value)
            .Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
        return connection.GetChanges() != 0;
    }

    bool CheckpointArgumentsTable::UpdateArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name, bool value)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Update(s_CheckpointArgumentsTable_Table_Name).Set()
            .Column(name).Equals(value)
            .Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
        return connection.GetChanges() != 0;
    }


    bool CheckpointArgumentsTable::ContainsArgument(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_CheckpointArgumentsTable_Table_Name)
            .Where(SQLite::RowIDName).Equals(id).And(name).IsNotNull();

        SQLite::Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int64_t>(0) != 0;
    }

    std::string CheckpointArgumentsTable::GetStringArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(name).From(s_CheckpointArgumentsTable_Table_Name).
            Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement statement = builder.Prepare(connection);
        if (statement.Step())
        {
            return statement.GetColumn<std::string>(0);
        }
        else
        {
            return {};
        }
    }

    bool CheckpointArgumentsTable::GetBoolArgumentById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view name)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(name).From(s_CheckpointArgumentsTable_Table_Name).
            Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement statement = builder.Prepare(connection);
        if (statement.Step())
        {
            return statement.GetColumn<bool>(0);
        }
        else
        {
            return {};
        }
    }

    int CheckpointArgumentsTable::GetFirstContextId(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(s_CheckpointArgumentsTable_ContextId_Column).From(s_CheckpointArgumentsTable_Table_Name)
            .OrderBy(s_CheckpointArgumentsTable_ContextId_Column);
        
        SQLite::Statement statement = builder.Prepare(connection);
        THROW_HR_IF(E_UNEXPECTED, !statement.Step());
        return statement.GetColumn<int>(0);
    }

    bool CheckpointArgumentsTable::SetCommandNameById(SQLite::Connection& connection, SQLite::rowid_t id, std::string_view commandName)
    {
        return UpdateArgumentById(connection, id, s_CheckpointArgumentsTable_CommandName_Column, commandName);
    }

    std::string CheckpointArgumentsTable::GetCommandNameById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        return GetStringArgumentById(connection, id, s_CheckpointArgumentsTable_CommandName_Column);
    }
}