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

    void QueueTable::AddQueueItem(const GUID& instanceIdentifier, const std::string& objectName)
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

    void QueueTable::SetActiveQueueItem(const std::string& objectName)
    {
        StatementBuilder builder;
        builder.Update(s_QueueTable_Table).Set().Column(s_QueueTable_Column_Active).Equals(true).Where(s_QueueTable_Column_ObjectName).Equals(objectName);

        builder.Execute(m_connection);
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, bool>> QueueTable::GetQueueItems()
    {
        StatementBuilder builder;
        builder.Select({
            s_QueueTable_Column_SetInstanceIdentifier,
            s_QueueTable_Column_ObjectName,
            s_QueueTable_Column_QueuedAt,
            s_QueueTable_Column_Active
        }).From(s_QueueTable_Table).OrderBy({ s_QueueTable_Column_QueuedAt, RowIDName });

        Statement statement = builder.Prepare(m_connection);

        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, bool>> result;

        while (statement.Step())
        {
            result.emplace_back(std::make_tuple(statement.GetColumn<GUID>(0), statement.GetColumn<std::string>(1), ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(2)), statement.GetColumn<bool>(3)));
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
