// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "StatusItemTable.h"
#include "Database/Schema/0_1/SetInfoTable.h"
#include <AppInstallerDateTime.h>
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::SQLite::Builder;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_3
{
    namespace
    {
        constexpr std::string_view s_StatusItemTable_Table = "status_items"sv;
        constexpr std::string_view s_StatusItemTable_ChangeIdentifierIndex = "status_items_change_idx"sv;
        constexpr std::string_view s_StatusItemTable_SetRowIdIndex = "status_items_set_idx"sv;
        constexpr std::string_view s_StatusItemTable_UnitInstanceIndex = "status_items_unit_idx"sv;

        constexpr std::string_view s_StatusItemTable_Column_ChangeIdentifier = "change_identifier"sv;
        constexpr std::string_view s_StatusItemTable_Column_ChangeTime = "change_time"sv;
        constexpr std::string_view s_StatusItemTable_Column_SetRowId = "set_rowid"sv;
        constexpr std::string_view s_StatusItemTable_Column_InQueue = "in_queue"sv;
        constexpr std::string_view s_StatusItemTable_Column_UnitInstanceIdentifier = "unit_instance_identifier"sv;
        constexpr std::string_view s_StatusItemTable_Column_State = "state"sv;
        constexpr std::string_view s_StatusItemTable_Column_ResultCode = "result_code"sv;
        constexpr std::string_view s_StatusItemTable_Column_ResultDescription = "result_description"sv;
        constexpr std::string_view s_StatusItemTable_Column_ResultDetails = "result_details"sv;
        constexpr std::string_view s_StatusItemTable_Column_ResultSource = "result_source"sv;

        void BuildBaseStatusSelectStatement(StatementBuilder& builder)
        {
            builder.Select({
                s_StatusItemTable_Column_ChangeIdentifier,          // 0
                s_StatusItemTable_Column_ChangeTime,                // 1
                V0_1::SetInfoTable::InstanceIdentifierColumn(),     // 2
                s_StatusItemTable_Column_InQueue,                   // 3
                s_StatusItemTable_Column_UnitInstanceIdentifier,    // 4
                s_StatusItemTable_Column_State,                     // 5
                s_StatusItemTable_Column_ResultCode,                // 6
                s_StatusItemTable_Column_ResultDescription,         // 7
                s_StatusItemTable_Column_ResultDetails,             // 8
                s_StatusItemTable_Column_ResultSource,              // 9
            }).From(s_StatusItemTable_Table).LeftOuterJoin(V0_1::SetInfoTable::TableName()).On(QualifiedColumn{ s_StatusItemTable_Table, s_StatusItemTable_Column_SetRowId }, QualifiedColumn{ V0_1::SetInfoTable::TableName(), RowIDName });
        }

        IConfigurationDatabase::StatusItemTuple GetTupleFromStatement(Statement& statement)
        {
            return std::make_tuple(
                statement.GetColumn<int64_t>(0),
                ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(1)),
                statement.GetColumn<GUID>(2),
                statement.GetColumn<bool>(3),
                statement.GetColumnIsNull(4) ? std::nullopt : std::make_optional<GUID>(statement.GetColumn<GUID>(4)),
                statement.GetColumn<int32_t>(5),
                statement.GetColumnIsNull(6) ? std::nullopt : std::make_optional<HRESULT>(statement.GetColumn<int32_t>(6)),
                statement.GetColumnIsNull(7) ? std::string{} : statement.GetColumn<std::string>(7),
                statement.GetColumnIsNull(8) ? std::string{} : statement.GetColumn<std::string>(8),
                statement.GetColumnIsNull(9) ? ConfigurationUnitResultSource::None : statement.GetColumn<ConfigurationUnitResultSource>(9)
            );
        }
    }

    StatusItemTable::StatusItemTable(Connection& connection) : m_connection(connection) {}

    void StatusItemTable::Create()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "StatusItemTable_Create_0_3");

        StatementBuilder tableBuilder;
        tableBuilder.CreateTable(s_StatusItemTable_Table).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_StatusItemTable_Column_ChangeIdentifier, Type::Int64).NotNull(),
            ColumnBuilder(s_StatusItemTable_Column_ChangeTime, Type::Int64).NotNull(),
            ColumnBuilder(s_StatusItemTable_Column_SetRowId, Type::RowId).NotNull(),
            ColumnBuilder(s_StatusItemTable_Column_InQueue, Type::Bool).NotNull(),
            ColumnBuilder(s_StatusItemTable_Column_UnitInstanceIdentifier, Type::Blob),
            ColumnBuilder(s_StatusItemTable_Column_State, Type::Int).NotNull(),
            ColumnBuilder(s_StatusItemTable_Column_ResultCode, Type::Int),
            ColumnBuilder(s_StatusItemTable_Column_ResultDescription, Type::Text),
            ColumnBuilder(s_StatusItemTable_Column_ResultDetails, Type::Text),
            ColumnBuilder(s_StatusItemTable_Column_ResultSource, Type::Int),
        });

        tableBuilder.Execute(m_connection);

        {
            StatementBuilder indexBuilder;
            indexBuilder.CreateIndex(s_StatusItemTable_ChangeIdentifierIndex).On(s_StatusItemTable_Table).Columns(s_StatusItemTable_Column_ChangeIdentifier);
            indexBuilder.Execute(m_connection);
        }

        {
            StatementBuilder indexBuilder;
            indexBuilder.CreateIndex(s_StatusItemTable_SetRowIdIndex).On(s_StatusItemTable_Table).Columns(s_StatusItemTable_Column_SetRowId);
            indexBuilder.Execute(m_connection);
        }

        {
            StatementBuilder indexBuilder;
            indexBuilder.CreateUniqueIndex(s_StatusItemTable_UnitInstanceIndex).On(s_StatusItemTable_Table).Columns(s_StatusItemTable_Column_UnitInstanceIdentifier);
            indexBuilder.Execute(m_connection);
        }

        savepoint.Commit();
    }

    void StatusItemTable::RemoveForSet(AppInstaller::SQLite::rowid_t target)
    {
        StatementBuilder builder;
        builder.DeleteFrom(s_StatusItemTable_Table).Where(s_StatusItemTable_Column_SetRowId).Equals(target);
        builder.Execute(m_connection);
    }

    std::vector<IConfigurationDatabase::StatusItemTuple> StatusItemTable::GetStatusSince(int64_t changeIdentifier)
    {
        StatementBuilder builder;
        BuildBaseStatusSelectStatement(builder);
        builder.Where(s_StatusItemTable_Column_ChangeIdentifier).IsGreaterThan(changeIdentifier).OrderBy(s_StatusItemTable_Column_ChangeIdentifier);

        Statement statement = builder.Prepare(m_connection);

        std::vector<IConfigurationDatabase::StatusItemTuple> result;

        while (statement.Step())
        {
            result.emplace_back(GetTupleFromStatement(statement));
        }

        return result;
    }

    std::tuple<int64_t, std::vector<IConfigurationDatabase::StatusItemTuple>> StatusItemTable::GetStatusBaseline()
    {
        int64_t latestChange = 0;
        std::vector<IConfigurationDatabase::StatusItemTuple> setStatus;

        StatementBuilder getLatestChangeBuilder;
        getLatestChangeBuilder.Select().Max(s_StatusItemTable_Column_ChangeIdentifier).From(s_StatusItemTable_Table);

        Statement getLatestChange = getLatestChangeBuilder.Prepare(m_connection);

        if (getLatestChange.Step())
        {
            latestChange = getLatestChange.GetColumn<int64_t>(0);
        }

        StatementBuilder builder;
        BuildBaseStatusSelectStatement(builder);
        builder.Where(s_StatusItemTable_Column_UnitInstanceIdentifier).IsNull();

        Statement statement = builder.Prepare(m_connection);

        while (statement.Step())
        {
            setStatus.emplace_back(GetTupleFromStatement(statement));
        }

        return std::make_tuple(latestChange, std::move(setStatus));
    }

    void StatusItemTable::UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state)
    {
        UNREFERENCED_PARAMETER(setInstanceIdentifier);
        UNREFERENCED_PARAMETER(state);
        // update status_items set state = 1 from (select rowid from set_info where instance_identifier = "foo") as sub where status_items.set_rowid = sub.rowid
        // check change count, if 0 then insert
        //StatementBuilder builder;
        //builder.Update(s_StatusItemTable_Table).Set().Column(s_StatusItemTable_Column_State).Equals(state).From(s_StatusItemTable_Table);
        THROW_HR(E_NOTIMPL);
    }

    void StatusItemTable::UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue)
    {
        UNREFERENCED_PARAMETER(setInstanceIdentifier);
        UNREFERENCED_PARAMETER(inQueue);
        THROW_HR(E_NOTIMPL);
    }

    void StatusItemTable::UpdateUnitState(const guid& setInstanceIdentifier, const IConfigurationDatabase::ConfigurationSetChangeDataPtr& changeData)
    {
        UNREFERENCED_PARAMETER(setInstanceIdentifier);
        UNREFERENCED_PARAMETER(changeData);
        THROW_HR(E_NOTIMPL);
    }

    ConfigurationSetState StatusItemTable::GetSetState(const guid& instanceIdentifier)
    {
        UNREFERENCED_PARAMETER(instanceIdentifier);
        THROW_HR(E_NOTIMPL);
    }

    std::chrono::system_clock::time_point StatusItemTable::GetSetFirstApply(const guid& instanceIdentifier)
    {
        UNREFERENCED_PARAMETER(instanceIdentifier);
        THROW_HR(E_NOTIMPL);
    }

    std::chrono::system_clock::time_point StatusItemTable::GetSetApplyBegun(const guid& instanceIdentifier)
    {
        UNREFERENCED_PARAMETER(instanceIdentifier);
        THROW_HR(E_NOTIMPL);
    }

    std::chrono::system_clock::time_point StatusItemTable::GetSetApplyEnded(const guid& instanceIdentifier)
    {
        UNREFERENCED_PARAMETER(instanceIdentifier);
        THROW_HR(E_NOTIMPL);
    }

    ConfigurationUnitState StatusItemTable::GetUnitState(const guid& instanceIdentifier)
    {
        UNREFERENCED_PARAMETER(instanceIdentifier);
        THROW_HR(E_NOTIMPL);
    }

    std::optional<std::tuple<HRESULT, std::string, std::string, ConfigurationUnitResultSource>> StatusItemTable::GetUnitResultInformation(const guid& instanceIdentifier)
    {
        UNREFERENCED_PARAMETER(instanceIdentifier);
        THROW_HR(E_NOTIMPL);
    }
}
