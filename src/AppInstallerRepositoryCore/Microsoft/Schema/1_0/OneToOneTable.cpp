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

        std::optional<SQLite::rowid_t> OneToOneTableSelectIdByValue(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value, bool useLike)
        {
            SQLite::Builder::StatementBuilder selectBuilder;
            selectBuilder.Select(SQLite::RowIDName).From(tableName).Where(valueName);

            if (useLike)
            {
                selectBuilder.LikeWithEscape(value);
            }
            else
            {
                selectBuilder.Equals(value);
            }

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

        std::vector<SQLite::rowid_t> OneToOneTableGetAllRowIds(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, size_t limit)
        {
            SQLite::Builder::StatementBuilder selectBuilder;
            selectBuilder.Select(SQLite::RowIDName).From(tableName).OrderBy(valueName);

            if (limit)
            {
                selectBuilder.Limit(limit);
            }

            SQLite::Statement select = selectBuilder.Prepare(connection);

            std::vector<SQLite::rowid_t> result;
            while (select.Step())
            {
                result.emplace_back(select.GetColumn<SQLite::rowid_t>(0));
            }
            return result;
        }

        SQLite::rowid_t OneToOneTableEnsureExists(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, std::string_view value, bool overwriteLikeMatch)
        {
            auto selectResult = OneToOneTableSelectIdByValue(connection, tableName, valueName, value, overwriteLikeMatch);
            if (selectResult)
            {
                if (overwriteLikeMatch)
                {
                    // If the value in the table is not an exact match, overwrite it with the incoming value
                    auto tableValue = OneToOneTableSelectValueById(connection, tableName, valueName, selectResult.value());
                    if (tableValue.value() != value)
                    {
                        SQLite::Builder::StatementBuilder updateBuilder;
                        updateBuilder.Update(tableName).Set().Column(valueName).Equals(value).Where(SQLite::RowIDName).Equals(selectResult);

                        updateBuilder.Execute(connection);
                    }
                }

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

        uint64_t OneToOneTableGetCount(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(SQLite::Builder::RowCount).From(tableName);

            SQLite::Statement countStatement = builder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            return static_cast<uint64_t>(countStatement.GetColumn<SQLite::rowid_t>(0));
        }

        bool OneToOneTableIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            return (OneToOneTableGetCount(connection, tableName) == 0);
        }
    }
}
