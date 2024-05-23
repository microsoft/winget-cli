// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SetInfoTable.h"
#include "ValueSetTable.h"
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
            ColumnBuilder(s_SetInfoTable_Column_Metadata, Type::RowId).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Parameters, Type::Text).NotNull(),
            ColumnBuilder(s_SetInfoTable_Column_Variables, Type::RowId).NotNull(),
        });

        tableBuilder.Execute(m_connection);

        savepoint.Commit();
    }

    rowid_t SetInfoTable::Add(const Configuration::ConfigurationSet& configurationSet)
    {
        Savepoint savepoint = Savepoint::Create(m_connection, "SetInfoTable_Add_0_1");

        THROW_HR_IF(E_NOTIMPL, configurationSet.Parameters().Size() > 0);

        ValueSetTable valueSetTable(m_connection);

        StatementBuilder builder;
        builder.InsertInto(s_SetInfoTable_Table).Columns({
            s_SetInfoTable_Column_InstanceIdentifier,
            s_SetInfoTable_Column_Name,
            s_SetInfoTable_Column_Origin,
            s_SetInfoTable_Column_Path,
            s_SetInfoTable_Column_FirstApply,
            s_SetInfoTable_Column_SchemaVersion,
            s_SetInfoTable_Column_Metadata,
            s_SetInfoTable_Column_Variables,
        }).Values(
            static_cast<GUID>(configurationSet.InstanceIdentifier()),
            ConvertToUTF8(configurationSet.Name()),
            ConvertToUTF8(configurationSet.Origin()),
            ConvertToUTF8(configurationSet.Path()),
            GetCurrentUnixEpoch(),
            ConvertToUTF8(configurationSet.SchemaVersion()),
            valueSetTable.Add(configurationSet.Metadata()),
            valueSetTable.Add(configurationSet.Variables())
        );

        builder.Execute(m_connection);
        rowid_t result = m_connection.GetLastInsertRowID();

        savepoint.Commit();
        return result;
    }

    Statement SetInfoTable::GetAllSetsStatement()
    {
        StatementBuilder builder;
        builder.Select({
            RowIDName,
            s_SetInfoTable_Column_InstanceIdentifier,
            s_SetInfoTable_Column_Name,
            s_SetInfoTable_Column_Origin,
            s_SetInfoTable_Column_Path,
            s_SetInfoTable_Column_FirstApply,
            s_SetInfoTable_Column_SchemaVersion,
            s_SetInfoTable_Column_Metadata,
            s_SetInfoTable_Column_Parameters,
            s_SetInfoTable_Column_Variables,
        }).From(s_SetInfoTable_Table);

        return builder.Prepare(m_connection);
    }
}
