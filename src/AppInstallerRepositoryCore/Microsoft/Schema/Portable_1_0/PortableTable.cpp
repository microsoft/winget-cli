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

    static constexpr std::string_view s_PortableTable_FilePath_Column = "filePath"sv;
    static constexpr std::string_view s_PortableTable_FileType_Column = "fileType"sv;
    static constexpr std::string_view s_PortableTable_SHA256_Column = "sha256"sv;
    static constexpr std::string_view s_PortableTable_SymlinkTarget_Column = "symlinkTarget"sv;
    static constexpr std::string_view s_PortableTable_IsCreated_Column = "isCreated"sv;

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
        createTableBuilder.Column(ColumnBuilder(s_PortableTable_FilePath_Column, Type::Text).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_PortableTable_FileType_Column, Type::Int64).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_PortableTable_SHA256_Column, Type::Blob).NotNull());
        createTableBuilder.Column(ColumnBuilder(s_PortableTable_SymlinkTarget_Column, Type::Text));
        createTableBuilder.Column(ColumnBuilder(s_PortableTable_IsCreated_Column, Type::Bool).NotNull());
        createTableBuilder.EndColumns();
        createTableBuilder.Execute(connection);

        savepoint.Commit();
    }

    std::optional<SQLite::rowid_t> PortableTable::SelectByFilePath(const SQLite::Connection& connection, const std::filesystem::path& path)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::RowIDName).From(s_PortableTable_Table_Name).Where(path.u8string());
        builder.Equals(path.u8string());

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

    void PortableTable::RemovePortableFileById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(s_PortableTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

        builder.Execute(connection);
    }

    SQLite::rowid_t PortableTable::Insert(SQLite::Connection& connection, const PortableFile& file)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.InsertInto(s_PortableTable_Table_Name)
            .Columns({ s_PortableTable_FilePath_Column,
                s_PortableTable_FileType_Column,
                s_PortableTable_SHA256_Column,
                s_PortableTable_SymlinkTarget_Column,
                s_PortableTable_IsCreated_Column })
            .Values(file.FilePath, file.FileType, file.SHA256, file.SymlinkTarget, file.IsCreated);

        builder.Execute(connection);
        return connection.GetLastInsertRowID();
    }

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
