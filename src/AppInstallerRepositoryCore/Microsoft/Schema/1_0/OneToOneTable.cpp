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

        SQLite::rowid_t OneToOneTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value)
        {
            {
                SQLite::Builder::StatementBuilder selectBuilder;
                selectBuilder.Select(SQLite::RowIDName).From(tableName).Where(valueName).Equals(value);

                SQLite::Statement select = selectBuilder.Prepare(connection);

                if (select.Step())
                {
                    return select.GetColumn<SQLite::rowid_t>(0);
                }
            }

            std::ostringstream insertSQL;
            insertSQL << "INSERT INTO [" << tableName << "] ([" << valueName << "]) VALUES (?)";

            SQLite::Statement insert = SQLite::Statement::Create(connection, insertSQL.str());

            insert.Bind(1, value);

            insert.Execute();

            return connection.GetLastInsertRowID();
        }

        void OneToOneTableDeleteIfNotNeededById(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, SQLite::rowid_t id)
        {
            // If a manifest is found that references this id, then we are done.
            if (ManifestTableSelectByValueId(connection, valueName, id))
            {
                return;
            }

            std::ostringstream deleteSQL;
            deleteSQL << "DELETE FROM [" << tableName << "] WHERE [" << SQLite::RowIDName << "] = ?";

            SQLite::Statement deleteStatement = SQLite::Statement::Create(connection, deleteSQL.str());

            deleteStatement.Bind(1, id);

            deleteStatement.Execute();
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
