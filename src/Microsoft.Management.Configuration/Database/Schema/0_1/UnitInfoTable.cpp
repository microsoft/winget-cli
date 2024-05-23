// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UnitInfoTable.h"
#include "ValueSetTable.h"
#include "ConfigurationUnit.h"
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerStrings.h>
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::SQLite::Builder;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    namespace
    {
        constexpr std::string_view s_UnitInfoTable_Table = "unit_info"sv;
        constexpr std::string_view s_UnitInfoTable_SetRowIdIndex = "unit_info_set_idx"sv;

        constexpr std::string_view s_UnitInfoTable_Column_SetRowId = "set_rowid"sv;
        constexpr std::string_view s_UnitInfoTable_Column_ParentRowId = "parent_rowid"sv;
        constexpr std::string_view s_UnitInfoTable_Column_InstanceIdentifier = "instance_identifier"sv;
        constexpr std::string_view s_UnitInfoTable_Column_Type = "type"sv;
        constexpr std::string_view s_UnitInfoTable_Column_Identifier = "identifier"sv;
        constexpr std::string_view s_UnitInfoTable_Column_Intent = "intent"sv;
        constexpr std::string_view s_UnitInfoTable_Column_Dependencies = "dependencies"sv;
        constexpr std::string_view s_UnitInfoTable_Column_Metadata = "metadata"sv;
        constexpr std::string_view s_UnitInfoTable_Column_Settings = "settings"sv;
        constexpr std::string_view s_UnitInfoTable_Column_IsActive = "is_active"sv;
        constexpr std::string_view s_UnitInfoTable_Column_IsGroup = "is_group"sv;
    }

    UnitInfoTable::UnitInfoTable(Connection& connection) : m_connection(connection) {}

    void UnitInfoTable::Create()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "UnitInfoTable_Create_0_1");

        StatementBuilder tableBuilder;
        tableBuilder.CreateTable(s_UnitInfoTable_Table).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_UnitInfoTable_Column_SetRowId, Type::RowId).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_ParentRowId, Type::RowId),
            ColumnBuilder(s_UnitInfoTable_Column_InstanceIdentifier, Type::Blob).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_Type, Type::Text).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_Identifier, Type::Text).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_Intent, Type::Int).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_Dependencies, Type::RowId).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_Metadata, Type::RowId).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_Settings, Type::RowId).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_IsActive, Type::Bool).NotNull(),
            ColumnBuilder(s_UnitInfoTable_Column_IsGroup, Type::Bool).NotNull(),
            });

        tableBuilder.Execute(m_connection);

        StatementBuilder indexBuilder;
        indexBuilder.CreateIndex(s_UnitInfoTable_SetRowIdIndex).On(s_UnitInfoTable_Table).Columns(s_UnitInfoTable_Column_SetRowId);

        indexBuilder.Execute(m_connection);

        savepoint.Commit();
    }

    void UnitInfoTable::Add(const Configuration::ConfigurationUnit& configurationUnit, AppInstaller::SQLite::rowid_t setRowId)
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "UnitInfoTable_Add_0_1");

        ValueSetTable valueSetTable(m_connection);

        StatementBuilder builder;
        builder.InsertInto(s_UnitInfoTable_Table).Columns({
            s_UnitInfoTable_Column_SetRowId,
            s_UnitInfoTable_Column_ParentRowId,
            s_UnitInfoTable_Column_InstanceIdentifier,
            s_UnitInfoTable_Column_Type,
            s_UnitInfoTable_Column_Identifier,
            s_UnitInfoTable_Column_Intent,
            s_UnitInfoTable_Column_Dependencies,
            s_UnitInfoTable_Column_Metadata,
            s_UnitInfoTable_Column_Settings,
            s_UnitInfoTable_Column_IsActive,
            s_UnitInfoTable_Column_IsGroup,
        }).Values(
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound,
            Unbound
        );

        Statement insertStatement = builder.Prepare(m_connection);

        struct UnitsToInsert
        {
            std::optional<rowid_t> Parent;
            Configuration::ConfigurationUnit Unit;
        };

        std::queue<UnitsToInsert> unitsToInsert;
        unitsToInsert.emplace(UnitsToInsert{ std::nullopt, configurationUnit });

        while (!unitsToInsert.empty())
        {
            const auto& current = unitsToInsert.front();

            insertStatement.Reset();

            bool isGroup = current.Unit.IsGroup();

            insertStatement.Bind(1, setRowId);
            insertStatement.Bind(2, current.Parent);
            insertStatement.Bind(3, static_cast<GUID>(current.Unit.InstanceIdentifier()));
            insertStatement.Bind(4, ConvertToUTF8(current.Unit.Type()));
            insertStatement.Bind(5, ConvertToUTF8(current.Unit.Identifier()));
            insertStatement.Bind(6, AppInstaller::ToIntegral(current.Unit.Intent()));
            insertStatement.Bind(7, valueSetTable.Add(current.Unit.Dependencies()));
            insertStatement.Bind(8, valueSetTable.Add(current.Unit.Metadata()));
            insertStatement.Bind(9, valueSetTable.Add(current.Unit.Settings()));
            insertStatement.Bind(10, current.Unit.IsActive());
            insertStatement.Bind(11, isGroup);

            insertStatement.Execute();

            if (isGroup)
            {
                rowid_t currentRowId = m_connection.GetLastInsertRowID();

                auto winrtUnits = current.Unit.Units();
                std::vector<ConfigurationUnit> units{ winrtUnits.Size() };
                winrtUnits.GetMany(0, units);

                for (const ConfigurationUnit& unit : units)
                {
                    unitsToInsert.emplace(UnitsToInsert{ currentRowId, unit });
                }
            }

            unitsToInsert.pop();
        }

        savepoint.Commit();
    }

    std::vector<IConfigurationDatabase::ConfigurationUnitPtr> UnitInfoTable::GetAllUnitsForSet(AppInstaller::SQLite::rowid_t setRowId)
    {
        StatementBuilder builder;
        builder.Select({
            RowIDName,                                 // 0
            s_UnitInfoTable_Column_ParentRowId,        // 1
            s_UnitInfoTable_Column_InstanceIdentifier, // 2
            s_UnitInfoTable_Column_Type,               // 3
            s_UnitInfoTable_Column_Identifier,         // 4
            s_UnitInfoTable_Column_Intent,             // 5
            s_UnitInfoTable_Column_Dependencies,       // 6
            s_UnitInfoTable_Column_Metadata,           // 7
            s_UnitInfoTable_Column_Settings,           // 8
            s_UnitInfoTable_Column_IsActive,           // 9
            s_UnitInfoTable_Column_IsGroup,            // 10
        }).From(s_UnitInfoTable_Table).Where(s_UnitInfoTable_Column_SetRowId).Equals(setRowId);

        Statement statement = builder.Prepare(m_connection);
        ValueSetTable valueSetTable(m_connection);

        std::vector<IConfigurationDatabase::ConfigurationUnitPtr> result;
        std::map<rowid_t, Configuration::ConfigurationUnit> rowToUnitMap;

        while (statement.Step())
        {
            auto unit = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnit>>(statement.GetColumn<GUID>(2));

            unit->Type(hstring{ ConvertToUTF16(statement.GetColumn<std::string>(3)) });
            unit->Identifier(hstring{ ConvertToUTF16(statement.GetColumn<std::string>(4)) });
            unit->Intent(statement.GetColumn<ConfigurationUnitIntent>(5));
            unit->Dependencies(valueSetTable.GetStringVector(statement.GetColumn<rowid_t>(6)));
            unit->Metadata(valueSetTable.GetValueSet(statement.GetColumn<rowid_t>(7)));
            unit->Settings(valueSetTable.GetValueSet(statement.GetColumn<rowid_t>(8)));
            unit->IsActive(statement.GetColumn<bool>(9));
            unit->IsGroup(statement.GetColumn<bool>(10));

            if (statement.GetColumnIsNull(1))
            {
                result.emplace_back(unit);
            }
            else
            {
                rowToUnitMap.at(statement.GetColumn<rowid_t>(1)).Units().Append(*unit);
            }

            rowToUnitMap.emplace(statement.GetColumn<rowid_t>(0), unit);
        }

        return result;
    }
}
