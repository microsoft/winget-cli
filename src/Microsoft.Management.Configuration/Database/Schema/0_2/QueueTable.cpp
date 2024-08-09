// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "QueueTable.h"
#include <AppInstallerDateTime.h>
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::SQLite::Builder;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_2
{
    namespace
    {
        constexpr std::string_view s_QueueTable_Table = "queue"sv;

        constexpr std::string_view s_QueueTable_Column_SetInstanceIdentifier = "set_instance_identifier"sv;
        constexpr std::string_view s_QueueTable_Column_ObjectName = "object_name"sv;
        constexpr std::string_view s_QueueTable_Column_QueuedAt = "queued_at"sv;
        constexpr std::string_view s_QueueTable_Column_Active = "active"sv;
        constexpr std::string_view s_QueueTable_Column_Process = "process"sv;
    }

    QueueTable::QueueTable(Connection& connection) : m_connection(connection) {}

    void QueueTable::Create()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "QueueTable_Create_0_2");

        StatementBuilder tableBuilder;
        tableBuilder.CreateTable(s_QueueTable_Table).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_QueueTable_Column_SetInstanceIdentifier, Type::Blob).NotNull(),
            ColumnBuilder(s_QueueTable_Column_ObjectName, Type::Text).Unique().NotNull(),
            ColumnBuilder(s_QueueTable_Column_QueuedAt, Type::Int64).NotNull(),
            ColumnBuilder(s_QueueTable_Column_Active, Type::Bool).NotNull(),
        });

        tableBuilder.Execute(m_connection);

        savepoint.Commit();
    }

    void QueueTable::AddProcessColumn()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "QueueTable_AddProcessColumn_0_3");

        StatementBuilder builder;
        builder.AlterTable(s_QueueTable_Table).Add(s_QueueTable_Column_Process, Type::Int64).NotNull().Default(0);

        builder.Execute(m_connection);

        savepoint.Commit();
    }

    void QueueTable::AddQueueItemWithoutProcess(const GUID& instanceIdentifier, const std::string& objectName)
    {
        StatementBuilder builder;
        builder.InsertInto(s_QueueTable_Table).Columns({
            s_QueueTable_Column_SetInstanceIdentifier,
            s_QueueTable_Column_ObjectName,
            s_QueueTable_Column_QueuedAt,
            s_QueueTable_Column_Active
        }).Values(
            instanceIdentifier,
            objectName,
            GetCurrentUnixEpoch(),
            false
        );

        builder.Execute(m_connection);
    }

    void QueueTable::AddQueueItemWithProcess(const GUID& instanceIdentifier, const std::string& objectName)
    {
        StatementBuilder builder;
        builder.InsertInto(s_QueueTable_Table).Columns({
            s_QueueTable_Column_SetInstanceIdentifier,
            s_QueueTable_Column_ObjectName,
            s_QueueTable_Column_QueuedAt,
            s_QueueTable_Column_Active,
            s_QueueTable_Column_Process
            }).Values(
                instanceIdentifier,
                objectName,
                GetCurrentUnixEpoch(),
                false,
                static_cast<int64_t>(GetCurrentProcessId())
            );

        builder.Execute(m_connection);
    }

    void QueueTable::SetActiveQueueItem(const std::string& objectName)
    {
        StatementBuilder builder;
        builder.Update(s_QueueTable_Table).Set().Column(s_QueueTable_Column_Active).Equals(true).Where(s_QueueTable_Column_ObjectName).Equals(objectName);

        builder.Execute(m_connection);
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> QueueTable::GetQueueItemsWithoutProcess()
    {
        StatementBuilder builder;
        builder.Select({
            s_QueueTable_Column_SetInstanceIdentifier,
            s_QueueTable_Column_ObjectName,
            s_QueueTable_Column_QueuedAt,
            s_QueueTable_Column_Active
            }).From(s_QueueTable_Table).OrderBy({ s_QueueTable_Column_QueuedAt, RowIDName });

        Statement statement = builder.Prepare(m_connection);

        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> result;

        while (statement.Step())
        {
            result.emplace_back(std::make_tuple(statement.GetColumn<GUID>(0), statement.GetColumn<std::string>(1), ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(2)), 0, statement.GetColumn<bool>(3)));
        }

        return result;
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> QueueTable::GetQueueItemsWithProcess()
    {
        StatementBuilder builder;
        builder.Select({
            s_QueueTable_Column_SetInstanceIdentifier,
            s_QueueTable_Column_ObjectName,
            s_QueueTable_Column_QueuedAt,
            s_QueueTable_Column_Active,
            s_QueueTable_Column_Process
        }).From(s_QueueTable_Table).OrderBy({ s_QueueTable_Column_QueuedAt, RowIDName });

        Statement statement = builder.Prepare(m_connection);

        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> result;

        while (statement.Step())
        {
            result.emplace_back(std::make_tuple(statement.GetColumn<GUID>(0), statement.GetColumn<std::string>(1), ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(2)), static_cast<DWORD>(statement.GetColumn<int64_t>(4)), statement.GetColumn<bool>(3)));
        }

        return result;
    }

    void QueueTable::RemoveQueueItem(const std::string& objectName)
    {
        StatementBuilder builder;
        builder.DeleteFrom(s_QueueTable_Table).Where(s_QueueTable_Column_ObjectName).Equals(objectName);

        builder.Execute(m_connection);
    }
}
