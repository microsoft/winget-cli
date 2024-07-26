// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "StatusItemTable.h"
#include "Database/Schema/0_1/SetInfoTable.h"
#include <AppInstallerDateTime.h>
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerStrings.h>
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
        constexpr std::string_view s_StatusItemTable_Column_ChangeTimeInitial = "change_time_initial"sv;
        constexpr std::string_view s_StatusItemTable_Column_ChangeTimeLatest = "change_time_latest"sv;
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
                s_StatusItemTable_Column_ChangeTimeLatest,          // 1
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

        int64_t GetLatestChangeIdentifier(Connection& connection)
        {
            StatementBuilder getLatestChangeBuilder;
            getLatestChangeBuilder.Select().Column(Aggregate::Max, s_StatusItemTable_Column_ChangeIdentifier).From(s_StatusItemTable_Table);

            Statement getLatestChange = getLatestChangeBuilder.Prepare(connection);

            return (getLatestChange.Step() ? getLatestChange.GetColumn<int64_t>(0) : 0);
        }

        int64_t GetNextChangeIdentifier(Connection& connection)
        {
            return GetLatestChangeIdentifier(connection) + 1;
        }

        void UpdateStatus(
            Connection& connection,
            const GUID& setInstanceIdentifier,
            const std::optional<int32_t>& state,
            const std::optional<bool>& inQueue,
            const std::optional<GUID>& unitInstanceIdentifier = std::nullopt,
            const std::optional<int32_t>& resultCode = std::nullopt,
            const std::optional<std::string>& resultDescription = std::nullopt,
            const std::optional<std::string>& resultDetails = std::nullopt,
            const std::optional<ConfigurationUnitResultSource>& resultSource = std::nullopt)
        {
            static constexpr std::string_view s_alias = "sub_expression";

            int64_t changeIdentifier = GetNextChangeIdentifier(connection);
            int64_t changeTime = GetCurrentUnixEpoch();

            // Statement like:
            // Update status_items set state = 1 from (Select rowid from set_info where instance_identifier = "foo") as sub where status_items.set_rowid = sub.rowid
            StatementBuilder updateBuilder;
            updateBuilder.Update(s_StatusItemTable_Table).Set();

            if (state)
            {
                updateBuilder.Column(s_StatusItemTable_Column_State).Equals(state.value());
            }

            if (inQueue)
            {
                updateBuilder.Column(s_StatusItemTable_Column_InQueue).Equals(inQueue.value());
            }

            if (resultCode)
            {
                updateBuilder.Column(s_StatusItemTable_Column_ResultCode).Equals(resultCode.value());
            }

            if (resultDescription)
            {
                updateBuilder.Column(s_StatusItemTable_Column_ResultDescription).Equals(resultDescription.value());
            }

            if (resultDetails)
            {
                updateBuilder.Column(s_StatusItemTable_Column_ResultDetails).Equals(resultDetails.value());
            }

            if (resultSource)
            {
                updateBuilder.Column(s_StatusItemTable_Column_ResultSource).Equals(resultSource.value());
            }

            updateBuilder.
                Column(s_StatusItemTable_Column_ChangeIdentifier).Equals(changeIdentifier).
                Column(s_StatusItemTable_Column_ChangeTimeLatest).Equals(changeTime).
            From().BeginParenthetical().
                Select(RowIDName).From(V0_1::SetInfoTable::TableName()).Where(V0_1::SetInfoTable::InstanceIdentifierColumn()).Equals(setInstanceIdentifier).
            EndParenthetical().As(s_alias).Where(QualifiedColumn{ s_StatusItemTable_Table, s_StatusItemTable_Column_SetRowId }).Equals(QualifiedColumn{ s_alias, RowIDName }).
                And(QualifiedColumn{ s_StatusItemTable_Table, s_StatusItemTable_Column_UnitInstanceIdentifier }).Equals(unitInstanceIdentifier);

            updateBuilder.Execute(connection);

            if (connection.GetChanges() == 0)
            {
                // No change; we need to insert the status row
                StatementBuilder insertBuilder;
                insertBuilder.InsertInto(s_StatusItemTable_Table).Columns({
                    s_StatusItemTable_Column_ChangeIdentifier,
                    s_StatusItemTable_Column_ChangeTimeInitial,
                    s_StatusItemTable_Column_ChangeTimeLatest,
                    s_StatusItemTable_Column_SetRowId,
                    s_StatusItemTable_Column_InQueue,
                    s_StatusItemTable_Column_UnitInstanceIdentifier,
                    s_StatusItemTable_Column_State,
                    s_StatusItemTable_Column_ResultCode,
                    s_StatusItemTable_Column_ResultDescription,
                    s_StatusItemTable_Column_ResultDetails,
                    s_StatusItemTable_Column_ResultSource,
                }).Select().
                    Value(changeIdentifier).
                    Value(changeTime).
                    Value(changeTime).
                    Column(QualifiedColumn{ V0_1::SetInfoTable::TableName(), RowIDName }).
                    Value(inQueue.value_or(false)).
                    Value(unitInstanceIdentifier).
                    Value(state.value_or(0)).
                    Value(resultCode).
                    Value(resultDescription).
                    Value(resultDetails).
                    Value(resultSource).
                From(V0_1::SetInfoTable::TableName()).Where(QualifiedColumn{ V0_1::SetInfoTable::TableName(), V0_1::SetInfoTable::InstanceIdentifierColumn() }).Equals(setInstanceIdentifier);

                insertBuilder.Execute(connection);
            }
        }

        Statement PrepareSelectStatusValues(Connection& connection, const std::optional<GUID>& setInstanceIdentifier, const std::optional<GUID>& unitInstanceIdentifier, std::initializer_list<std::string_view> columns)
        {
            THROW_HR_IF(E_INVALIDARG, (setInstanceIdentifier && unitInstanceIdentifier) || (!setInstanceIdentifier && !unitInstanceIdentifier));

            StatementBuilder builder;
            builder.Select(columns).From(s_StatusItemTable_Table);

            if (setInstanceIdentifier)
            {
                builder.Join(V0_1::SetInfoTable::TableName()).On(QualifiedColumn{ s_StatusItemTable_Table, s_StatusItemTable_Column_SetRowId }, QualifiedColumn{ V0_1::SetInfoTable::TableName(), RowIDName }).
                    Where(QualifiedColumn{ V0_1::SetInfoTable::TableName(), V0_1::SetInfoTable::InstanceIdentifierColumn() }).Equals(setInstanceIdentifier).
                    And(s_StatusItemTable_Column_UnitInstanceIdentifier).IsNull();
            }
            else
            {
                builder.Where(s_StatusItemTable_Column_UnitInstanceIdentifier).Equals(unitInstanceIdentifier.value());
            }

            return builder.Prepare(connection);
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
            ColumnBuilder(s_StatusItemTable_Column_ChangeTimeInitial, Type::Int64).NotNull(),
            ColumnBuilder(s_StatusItemTable_Column_ChangeTimeLatest, Type::Int64).NotNull(),
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
        int64_t latestChange = GetLatestChangeIdentifier(m_connection);
        std::vector<IConfigurationDatabase::StatusItemTuple> setStatus;

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
        UpdateStatus(m_connection, setInstanceIdentifier, AppInstaller::ToIntegral(state), std::nullopt);
    }

    void StatusItemTable::UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue)
    {
        UpdateStatus(m_connection, setInstanceIdentifier, std::nullopt, inQueue);
    }

    void StatusItemTable::UpdateUnitState(const guid& setInstanceIdentifier, const IConfigurationDatabase::ConfigurationSetChangeDataPtr& changeData)
    {
        const auto& resultInformation = changeData->ResultInformation();

        std::optional<HRESULT> resultCode;
        std::optional<std::string> resultDescription;
        std::optional<std::string> resultDetails;
        std::optional<ConfigurationUnitResultSource> resultSource;

        if (resultInformation)
        {
            resultCode = resultInformation.ResultCode();
            resultDescription = ConvertToUTF8(resultInformation.Description());
            resultDetails = ConvertToUTF8(resultInformation.Details());
            resultSource = resultInformation.ResultSource();
        }

        UpdateStatus(m_connection, setInstanceIdentifier, AppInstaller::ToIntegral(changeData->UnitState()), std::nullopt, changeData->Unit().InstanceIdentifier(), resultCode, resultDescription, resultDetails, resultSource);
    }

    ConfigurationSetState StatusItemTable::GetSetState(const guid& instanceIdentifier)
    {
        Statement statement = PrepareSelectStatusValues(m_connection, instanceIdentifier, std::nullopt, { s_StatusItemTable_Column_State });

        return (statement.Step() ? statement.GetColumn<ConfigurationSetState>(0) : ConfigurationSetState::Unknown);
    }

    std::chrono::system_clock::time_point StatusItemTable::GetSetApplyBegun(const GUID& instanceIdentifier)
    {
        Statement statement = PrepareSelectStatusValues(m_connection, instanceIdentifier, std::nullopt, { s_StatusItemTable_Column_ChangeTimeInitial });

        return (statement.Step() ? ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(0)) : std::chrono::system_clock::time_point{});
    }

    std::chrono::system_clock::time_point StatusItemTable::GetSetApplyEnded(const GUID& instanceIdentifier)
    {
        Statement statement = PrepareSelectStatusValues(m_connection, instanceIdentifier, std::nullopt, { s_StatusItemTable_Column_ChangeTimeLatest, s_StatusItemTable_Column_InQueue });

        // Only return the end time if no longer in the queue
        if (statement.Step() && !statement.GetColumn<bool>(1))
        {
            return ConvertUnixEpochToSystemClock(statement.GetColumn<int64_t>(0));
        }

        return std::chrono::system_clock::time_point{};
    }

    ConfigurationUnitState StatusItemTable::GetUnitState(const guid& instanceIdentifier)
    {
        Statement statement = PrepareSelectStatusValues(m_connection, std::nullopt, instanceIdentifier, { s_StatusItemTable_Column_State });

        return (statement.Step() ? statement.GetColumn<ConfigurationUnitState>(0) : ConfigurationUnitState::Unknown);
    }

    std::optional<std::tuple<HRESULT, std::string, std::string, ConfigurationUnitResultSource>> StatusItemTable::GetUnitResultInformation(const guid& instanceIdentifier)
    {
        Statement statement = PrepareSelectStatusValues(m_connection, std::nullopt, instanceIdentifier,
            { s_StatusItemTable_Column_ResultCode, s_StatusItemTable_Column_ResultDescription, s_StatusItemTable_Column_ResultDetails, s_StatusItemTable_Column_ResultSource });

        if (statement.Step() && !statement.GetColumnIsNull(0))
        {
            return statement.GetRow<int32_t, std::string, std::string, ConfigurationUnitResultSource>();
        }

        return std::nullopt;
    }
}
