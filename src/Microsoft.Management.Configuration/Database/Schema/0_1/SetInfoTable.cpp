// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SetInfoTable.h"
#include "UnitInfoTable.h"
#include "ConfigurationSetSerializer.h"
#include "ConfigurationSetParser.h"
#include <AppInstallerDateTime.h>
#include <AppInstallerStrings.h>
#include <winget/SQLiteStatementBuilder.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::SQLite::Builder;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    namespace
    {
        constexpr std::string_view s_SetInfoTable_Table = "set_info"sv;

        constexpr std::string_view s_SetInfoTable_Column_InstanceIdentifier = "instance_identifier"sv;
        constexpr std::string_view s_SetInfoTable_Column_Name = "name"sv;
        constexpr std::string_view s_SetInfoTable_Column_Origin = "origin"sv;
        constexpr std::string_view s_SetInfoTable_Column_Path = "path"sv;
        constexpr std::string_view s_SetInfoTable_Column_FirstApply = "first_apply"sv;
        constexpr std::string_view s_SetInfoTable_Column_SchemaVersion = "schema_version"sv;
        constexpr std::string_view s_SetInfoTable_Column_Metadata = "metadata"sv;
        constexpr std::string_view s_SetInfoTable_Column_Parameters = "parameters"sv;
        constexpr std::string_view s_SetInfoTable_Column_Variables = "variables"sv;
    }

    SetInfoTable::SetInfoTable(Connection& connection) : m_connection(connection) {}

    void SetInfoTable::Create()
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "SetInfoTable_Create_0_1");

        StatementBuilder tableBuilder;
        tableBuilder.CreateTable(s_SetInfoTable_Table).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_SetInfoTable_Column_InstanceIdentifier, Type::Blob).Unique().NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Name, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Origin, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Path, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_FirstApply, Type::Int64).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_SchemaVersion, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Metadata, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Parameters, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Variables, Type::Text).NotNull(),
        });

        tableBuilder.Execute(m_connection);

        savepoint.Commit();
    }

    rowid_t SetInfoTable::Add(const Configuration::ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOTIMPL, configurationSet.Parameters().Size() > 0);

        Savepoint savepoint = Savepoint::Create(m_connection, "SetInfoTable_Add_0_1");

        hstring schemaVersion = configurationSet.SchemaVersion();
        auto serializer = ConfigurationSetSerializer::CreateSerializer(schemaVersion, true);

        StatementBuilder builder;
        builder.InsertInto(s_SetInfoTable_Table).Columns({
            s_SetInfoTable_Column_InstanceIdentifier,
            s_SetInfoTable_Column_Name,
            s_SetInfoTable_Column_Origin,
            s_SetInfoTable_Column_Path,
            s_SetInfoTable_Column_FirstApply,
            s_SetInfoTable_Column_SchemaVersion,
            s_SetInfoTable_Column_Metadata,
            s_SetInfoTable_Column_Parameters,
            s_SetInfoTable_Column_Variables,
        }).Values(
            static_cast<GUID>(configurationSet.InstanceIdentifier()),
            ConvertToUTF8(configurationSet.Name()),
            ConvertToUTF8(configurationSet.Origin()),
            ConvertToUTF8(configurationSet.Path()),
            GetCurrentUnixEpoch(),
            ConvertToUTF8(schemaVersion),
            serializer->SerializeValueSet(configurationSet.Metadata()),
            std::string{}, // Parameters
            serializer->SerializeValueSet(configurationSet.Variables())
        );

        builder.Execute(m_connection);
        rowid_t result = m_connection.GetLastInsertRowID();

        UnitInfoTable unitInfoTable(m_connection);

        auto winrtUnits = configurationSet.Units();
        std::vector<Configuration::ConfigurationUnit> units{ winrtUnits.Size() };
        winrtUnits.GetMany(0, units);

        for (const auto& unit : units)
        {
            unitInfoTable.Add(unit, result, schemaVersion);
        }

        savepoint.Commit();
        return result;
    }

    void SetInfoTable::Update(rowid_t target, const Configuration::ConfigurationSet& configurationSet)
    {
        THROW_HR_IF(E_NOTIMPL, configurationSet.Parameters().Size() > 0);

        Savepoint savepoint = Savepoint::Create(m_connection, "SetInfoTable_Update_0_1");

        hstring schemaVersion = configurationSet.SchemaVersion();
        auto serializer = ConfigurationSetSerializer::CreateSerializer(schemaVersion, true);

        StatementBuilder builder;
        builder.Update(s_SetInfoTable_Table).Set().
            Column(s_SetInfoTable_Column_Name).Equals(ConvertToUTF8(configurationSet.Name())).
            Column(s_SetInfoTable_Column_Origin).Equals(ConvertToUTF8(configurationSet.Origin())).
            Column(s_SetInfoTable_Column_Path).Equals(ConvertToUTF8(configurationSet.Path())).
            Column(s_SetInfoTable_Column_SchemaVersion).Equals(ConvertToUTF8(schemaVersion)).
            Column(s_SetInfoTable_Column_Metadata).Equals(serializer->SerializeValueSet(configurationSet.Metadata())).
            Column(s_SetInfoTable_Column_Variables).Equals(serializer->SerializeValueSet(configurationSet.Variables())).
        Where(RowIDName).Equals(target);

        builder.Execute(m_connection);

        UnitInfoTable unitInfoTable(m_connection);
        unitInfoTable.UpdateForSet(target, configurationSet.Units(), schemaVersion);

        savepoint.Commit();
    }

    void SetInfoTable::Remove(rowid_t target)
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "SetInfoTable_Remove_0_1");

        StatementBuilder builder;
        builder.DeleteFrom(s_SetInfoTable_Table).Where(RowIDName).Equals(target);
        builder.Execute(m_connection);

        UnitInfoTable unitInfoTable(m_connection);
        unitInfoTable.RemoveForSet(target);

        savepoint.Commit();
    }

    std::vector<IConfigurationDatabase::ConfigurationSetPtr> SetInfoTable::GetAllSets()
    {
        std::vector<IConfigurationDatabase::ConfigurationSetPtr> result;

        StatementBuilder builder;
        builder.Select({
            RowIDName,                                  // 0
            s_SetInfoTable_Column_InstanceIdentifier,   // 1
            s_SetInfoTable_Column_Name,                 // 2
            s_SetInfoTable_Column_Origin,               // 3
            s_SetInfoTable_Column_Path,                 // 4
            s_SetInfoTable_Column_FirstApply,           // 5
            s_SetInfoTable_Column_SchemaVersion,        // 6
            s_SetInfoTable_Column_Metadata,             // 7
            s_SetInfoTable_Column_Parameters,           // 8
            s_SetInfoTable_Column_Variables,            // 9
        }).From(s_SetInfoTable_Table);

        Statement getAllSets = builder.Prepare(m_connection);

        UnitInfoTable unitInfoTable(m_connection);

        while (getAllSets.Step())
        {
            auto configurationSet = make_self<implementation::ConfigurationSet>(getAllSets.GetColumn<GUID>(1));

            configurationSet->Name(hstring{ ConvertToUTF16(getAllSets.GetColumn<std::string>(2)) });
            configurationSet->Origin(hstring{ ConvertToUTF16(getAllSets.GetColumn<std::string>(3)) });
            configurationSet->Path(hstring{ ConvertToUTF16(getAllSets.GetColumn<std::string>(4)) });
            configurationSet->FirstApply(clock::from_sys(ConvertUnixEpochToSystemClock(getAllSets.GetColumn<int64_t>(5))));

            std::string schemaVersion = getAllSets.GetColumn<std::string>(6);
            configurationSet->SchemaVersion(hstring{ ConvertToUTF16(schemaVersion) });

            auto parser = ConfigurationSetParser::CreateForSchemaVersion(schemaVersion);
            configurationSet->Metadata(parser->ParseValueSet(getAllSets.GetColumn<std::string>(7)));
            THROW_HR_IF(E_NOTIMPL, !getAllSets.GetColumn<std::string>(8).empty());
            configurationSet->Variables(parser->ParseValueSet(getAllSets.GetColumn<std::string>(9)));

            std::vector<Configuration::ConfigurationUnit> winrtUnits;
            for (const auto& unit : unitInfoTable.GetAllUnitsForSet(getAllSets.GetColumn<rowid_t>(0), schemaVersion))
            {
                winrtUnits.emplace_back(*unit);
            }
            configurationSet->Units(std::move(winrtUnits));

            result.emplace_back(std::move(configurationSet));
        }

        return result;
    }

    std::optional<rowid_t> SetInfoTable::GetSetRowId(const GUID& instanceIdentifier)
    {
        StatementBuilder builder;
        builder.Select(RowIDName).From(s_SetInfoTable_Table).Where(s_SetInfoTable_Column_InstanceIdentifier).Equals(instanceIdentifier);

        Statement select = builder.Prepare(m_connection);

        if (select.Step())
        {
            return select.GetColumn<rowid_t>(0);
        }

        return std::nullopt;
    }
}
