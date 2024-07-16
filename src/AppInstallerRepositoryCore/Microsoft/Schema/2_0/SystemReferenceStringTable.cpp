// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/2_0/SystemReferenceStringTable.h"
#include "Microsoft/Schema/2_0/PackagesTable.h"
#include <winget/SQLiteStatementBuilder.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        using PrimaryTable = PackagesTable;

        using namespace std::string_view_literals;
        static constexpr std::string_view s_SystemReferenceStringTable_PrimaryName = "package"sv;

        std::string_view SystemReferenceStringTableGetPrimaryColumnName()
        {
            return s_SystemReferenceStringTable_PrimaryName;
        }

        void SystemReferenceStringTableCreate(SQLite::Connection& connection, std::string_view tableName, std::string_view valueName)
        {
            using namespace SQLite::Builder;

            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_create_v2_0");

            StatementBuilder createTableBuilder;
            createTableBuilder.CreateTable(tableName).Columns({
                ColumnBuilder(valueName, Type::Text).NotNull(),
                ColumnBuilder(s_SystemReferenceStringTable_PrimaryName, Type::RowId).NotNull(),
                PrimaryKeyBuilder({ valueName, s_SystemReferenceStringTable_PrimaryName })
                }).WithoutRowID();

            createTableBuilder.Execute(connection);

            savepoint.Commit();
        }

        void SystemReferenceStringTableDrop(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder dropTableBuilder;
            dropTableBuilder.DropTable(tableName);

            dropTableBuilder.Execute(connection);
        }

        std::vector<std::string> SystemReferenceStringTableGetValuesByPrimaryId(
            const SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            SQLite::rowid_t primaryId)
        {
            std::vector<std::string> result;

            SQLite::Builder::StatementBuilder builder;
            builder.Select(valueName).
                From(tableName).Where(s_SystemReferenceStringTable_PrimaryName).Equals(primaryId);

            SQLite::Statement statement = builder.Prepare(connection);

            while (statement.Step())
            {
                result.emplace_back(statement.GetColumn<std::string>(0));
            }

            return result;
        }

        void SystemReferenceStringTableEnsureExists(
            SQLite::Connection& connection,
            std::string_view tableName,
            std::string_view valueName,
            const std::vector<std::string>& values,
            SQLite::rowid_t primaryId)
        {
            SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, std::string{ tableName } + "_ensure_v2_0");

            SQLite::Builder::StatementBuilder builder;

            builder.InsertOrIgnore(tableName).
                Columns({ valueName, s_SystemReferenceStringTable_PrimaryName }).Values(SQLite::Builder::Unbound, primaryId);

            SQLite::Statement insertStatement = builder.Prepare(connection);

            for (const std::string& value : values)
            {
                // Second, insert into the mapping table
                insertStatement.Reset();
                insertStatement.Bind(1, value);

                insertStatement.Execute();
            }

            savepoint.Commit();
        }

        bool SystemReferenceStringTableCheckConsistency(const SQLite::Connection& connection, std::string_view tableName, std::string_view valueName, bool log)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            bool result = true;

            {
                // Build a select statement to find rows containing references to primaries with nonexistent rowids
                // Such as:
                // Select data.data, data.primary from data left outer join primary on data.primary = primary.rowid where primary.id is null

                SQLite::Builder::StatementBuilder builder;
                builder.
                    Select({ QCol(tableName, valueName), QCol(tableName, s_SystemReferenceStringTable_PrimaryName) }).
                    From(tableName).
                    LeftOuterJoin(details::PrimaryTable::TableName()).On(QCol(tableName, s_SystemReferenceStringTable_PrimaryName), QCol(details::PrimaryTable::TableName(), SQLite::RowIDName)).
                    Where(QCol(details::PrimaryTable::TableName(), SQLite::RowIDName)).IsNull();

                SQLite::Statement select = builder.Prepare(connection);

                while (select.Step())
                {
                    result = false;

                    if (!log)
                    {
                        break;
                    }

                    AICLI_LOG(Repo, Info, << "  [INVALID] " << tableName << " [" << select.GetColumn<std::string>(0) <<
                        ", " << select.GetColumn<SQLite::rowid_t>(1) << "] refers to invalid " << details::PrimaryTable::TableName());
                }
            }

            if (!result && !log)
            {
                return result;
            }

            // Build a select statement to find values that contain an embedded null character
            // Such as:
            // Select count(*) from table where instr(value,char(0))>0
            SQLite::Builder::StatementBuilder builder;
            builder.
                Select({ valueName, s_SystemReferenceStringTable_PrimaryName }).
                From(tableName).
                WhereValueContainsEmbeddedNullCharacter(valueName);

            SQLite::Statement select = builder.Prepare(connection);

            while (select.Step())
            {
                result = false;

                if (!log)
                {
                    break;
                }

                AICLI_LOG(Repo, Info, << "  [INVALID] value in table [" << tableName << "] for primary [" << select.GetColumn<SQLite::rowid_t>(1) << "] contains an embedded null character and starts with [" << select.GetColumn<std::string>(0) << "]");
            }

            return result;
        }

        bool SystemReferenceStringTableIsEmpty(SQLite::Connection& connection, std::string_view tableName)
        {
            SQLite::Builder::StatementBuilder countBuilder;
            countBuilder.Select(SQLite::Builder::RowCount).From(tableName);

            SQLite::Statement countStatement = countBuilder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

            return countStatement.GetColumn<int>(0) == 0;
        }

        int SystemReferenceStringTableBuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::string_view tableName,
            std::string_view valueName,
            std::string_view primaryAlias,
            std::string_view valueAlias,
            bool useLike)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            // Build a statement like:
            //      SELECT table.package as p, table.value as v from table
            //      where table.value = <value>
            builder.Select().
                Column(s_SystemReferenceStringTable_PrimaryName).As(primaryAlias).
                Column(valueName).As(valueAlias).
                From(tableName).
                Where(valueName);

            int result = -1;

            if (useLike)
            {
                builder.Like(SQLite::Builder::Unbound);
                result = builder.GetLastBindIndex();
                builder.Escape(SQLite::EscapeCharForLike);
            }
            else
            {
                builder.Equals(SQLite::Builder::Unbound);
                result = builder.GetLastBindIndex();
            }

            return result;
        }

        std::vector<int> SystemReferenceStringTableBuildPairedSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            std::string_view tableName,
            std::string_view valueName,
            std::string_view pairedTableName,
            std::string_view pairedValueName,
            std::string_view primaryAlias,
            std::string_view valueAlias,
            bool useLike)
        {
            using QCol = SQLite::Builder::QualifiedColumn;

            // Build a statement like:
            //      SELECT table.package as p, '' as v from table
            //      join paired on table.package = paired.package
            //      where table.value = <value1> and paired.pairedValue = <value2>
            builder.Select().
                Column(QCol(tableName, s_SystemReferenceStringTable_PrimaryName)).As(primaryAlias).
                Value(std::string_view{}).As(valueAlias).
                From(tableName).
                Join(pairedTableName).On(QCol(tableName, s_SystemReferenceStringTable_PrimaryName), QCol(pairedTableName, s_SystemReferenceStringTable_PrimaryName)).
                Where(QCol(tableName, valueName));

            std::vector<int> result;

            if (useLike)
            {
                builder.Like(SQLite::Builder::Unbound);
                result.push_back(builder.GetLastBindIndex());
                builder.Escape(SQLite::EscapeCharForLike);
            }
            else
            {
                builder.Equals(SQLite::Builder::Unbound);
                result.push_back(builder.GetLastBindIndex());
            }

            builder.And(QCol(pairedTableName, pairedValueName));

            if (useLike)
            {
                builder.Like(SQLite::Builder::Unbound);
                result.push_back(builder.GetLastBindIndex());
                builder.Escape(SQLite::EscapeCharForLike);
            }
            else
            {
                builder.Equals(SQLite::Builder::Unbound);
                result.push_back(builder.GetLastBindIndex());
            }

            return result;
        }
    }
}
