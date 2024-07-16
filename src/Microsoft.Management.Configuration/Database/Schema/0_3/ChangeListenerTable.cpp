// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ChangeListenerTable.h"
#include <AppInstallerDateTime.h>
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::SQLite::Builder;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_3
{
    namespace
    {
        constexpr std::string_view s_ChangeListenerTable_Table = "change_listeners"sv;

        constexpr std::string_view s_ChangeListenerTable_Column_ObjectName = "object_name"sv;
        constexpr std::string_view s_ChangeListenerTable_Column_StartedAt = "started_at"sv;
        constexpr std::string_view s_ChangeListenerTable_Column_Process = "process"sv;
    }

    ChangeListenerTable::ChangeListenerTable(Connection& connection) : m_connection(connection) {}

    void ChangeListenerTable::Create()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "ChangeListenerTable_Create_0_3");

        StatementBuilder tableBuilder;
        tableBuilder.CreateTable(s_ChangeListenerTable_Table).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_ChangeListenerTable_Column_ObjectName, Type::Text).Unique().NotNull(),
            ColumnBuilder(s_ChangeListenerTable_Column_StartedAt, Type::Int64).NotNull(),
            ColumnBuilder(s_ChangeListenerTable_Column_Process, Type::Int64).NotNull(),
        });

        tableBuilder.Execute(m_connection);

        savepoint.Commit();
    }

    void ChangeListenerTable::AddChangeListener(const std::string& objectName)
    {
        StatementBuilder builder;
        builder.InsertInto(s_ChangeListenerTable_Table).Columns({
            s_ChangeListenerTable_Column_ObjectName,
            s_ChangeListenerTable_Column_StartedAt,
            s_ChangeListenerTable_Column_Process
            }).Values(
                objectName,
                GetCurrentUnixEpoch(),
                static_cast<int64_t>(GetCurrentProcessId())
            );

        builder.Execute(m_connection);
    }

    void ChangeListenerTable::RemoveChangeListener(const std::string& objectName)
    {
        StatementBuilder builder;
        builder.DeleteFrom(s_ChangeListenerTable_Table).Where(s_ChangeListenerTable_Column_ObjectName).Equals(objectName);

        builder.Execute(m_connection);
    }

    std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, DWORD>> ChangeListenerTable::GetChangeListeners()
    {
        StatementBuilder builder;
        builder.Select({
            s_ChangeListenerTable_Column_ObjectName,
            s_ChangeListenerTable_Column_StartedAt,
            s_ChangeListenerTable_Column_Process
        }).From(s_ChangeListenerTable_Table);

        Statement statement = builder.Prepare(m_connection);

        std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, DWORD>> result;

        while (statement.Step())
        {
            result.emplace_back(std::make_tuple(statement.GetColumn<std::string>(0), ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(1)), static_cast<DWORD>(statement.GetColumn<int64_t>(2))));
        }

        return result;
    }
}
