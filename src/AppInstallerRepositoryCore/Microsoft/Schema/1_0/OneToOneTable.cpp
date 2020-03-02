// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/OneToOneTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace details
    {
        void CreateOneToOneTable(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
        {
            using namespace SQLite::Builder;

            StatementBuilder createTableBuilder;
            createTableBuilder.CreateTable(tableName).Columns({
                ColumnBuilder(valueName, Type::Text).NotNull().PrimaryKey()
                });

            createTableBuilder.Execute(connection);
        }

        std::optional<SQLite::rowid_t> OneToOneTableSelectIdByValue(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value)
        {
            SQLite::Builder::StatementBuilder selectBuilder;
            selectBuilder.Select(SQLite::RowIDName).From(tableName).Where(valueName).Equals(value);

            SQLite::Statement select = selectBuilder.Prepare(connection);

            if (select.Step())
            {
                return select.GetColumn<SQLite::rowid_t>(0);
            }
            else
            {
                return {};
            }
        }

        std::optional<std::string> OneToOneTableSelectValueById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id)
        {
            SQLite::Builder::StatementBuilder selectBuilder;
            selectBuilder.Select(valueName).From(tableName).Where(SQLite::RowIDName).Equals(id);

            SQLite::Statement select = selectBuilder.Prepare(connection);

            if (select.Step())
            {
                return select.GetColumn<std::string>(0);
            }
            else
            {
                return {};
            }
        }

        SQLite::rowid_t OneToOneTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value)
        {
            auto selectResult = OneToOneTableSelectIdByValue(connection, tableName, valueName, value);
            if (selectResult)
            {
                return selectResult.value();
            }

            SQLite::Builder::StatementBuilder insertBuilder;
            insertBuilder.InsertInto(tableName).Columns(valueName).Values(value);

            insertBuilder.Execute(connection);

            return connection.GetLastInsertRowID();
        }

        void OneToOneTableDeleteIfNotNeededById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id)
        {
            // If a manifest is found that references this id, then we are done.
            if (ManifestTableSelectByValueIds(connection, { valueName }, { id }))
            {
                return;
            }

            SQLite::Builder::StatementBuilder builder;
            builder.DeleteFrom(tableName).Where(SQLite::RowIDName).Equals(id);

            builder.Execute(connection);
        }

        bool OneToOneTableIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(SQLite::Builder::RowCount).From(tableName);

            SQLite::Statement countStatement = builder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            return (countStatement.GetColumn<int>(0) == 0);
        }
    }
}
