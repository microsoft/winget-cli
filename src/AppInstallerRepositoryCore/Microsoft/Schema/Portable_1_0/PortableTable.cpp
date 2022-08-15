// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableTable.h"
#include "SQLiteStatementBuilder.h"

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PortableTable_Table_Name = "portable"sv;
    static constexpr std::string_view s_PortableTable_Index_Separator = "_"sv;
    static constexpr std::string_view s_PortableTable_Index_Suffix = "_index"sv;

    std::string_view PortableTable::TableName()
    {
        return s_PortableTable_Table_Name;
    }
    
    void PortableTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createPortableTable_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_PortableTable_Table_Name).BeginColumns();

        // Add an integer primary key to keep the manifest rowid consistent
        createTableBuilder.Column(IntegerPrimaryKey());

        // Build each column for portable table
        createTableBuilder.Column(ColumnBuilder("filepath", Type::Text).NotNull());
        createTableBuilder.Column(ColumnBuilder("filetype", Type::Int64).NotNull());
        createTableBuilder.Column(ColumnBuilder("sha256", Type::Blob).NotNull());
        createTableBuilder.Column(ColumnBuilder("symlinktarget", Type::Text));
        createTableBuilder.Column(ColumnBuilder("iscreated", Type::Bool).NotNull());
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    //SQLite::rowid_t ManifestTable::Insert(SQLite::Connection& connection, std::initializer_list<ManifestOneToOneValue> values)
    //{
    //    SQLite::Builder::StatementBuilder builder;
    //    builder.InsertInto(s_ManifestTable_Table_Name).BeginColumns();

    //    for (const ManifestOneToOneValue& value : values)
    //    {
    //        builder.Column(value.Name);
    //    }

    //    builder.EndColumns().BeginValues();

    //    for (const ManifestOneToOneValue& value : values)
    //    {
    //        builder.Value(value.Value);
    //    }

    //    builder.EndValues();

    //    builder.Execute(connection);

    //    return connection.GetLastInsertRowID();
    //}

    bool PortableTable::ExistsById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_PortableTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) != 0);
    }

    void PortableTable::DeleteById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_PortableTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
    }

    bool PortableTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_PortableTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }
}
