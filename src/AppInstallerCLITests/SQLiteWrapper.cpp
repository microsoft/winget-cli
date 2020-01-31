// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <SQLiteWrapper.h>
#include <SQLiteStatementBuilder.h>

using namespace AppInstaller::Repository::SQLite;

static const char* s_firstColumn = "first";
static const char* s_secondColumn = "second";
static const char* s_tableName = "simpletest";

static const char* s_CreateSimpleTestTableSQL = R"(
CREATE TABLE [main].[simpletest](
  [first] INT, 
  [second] TEXT);
)";

static const char* s_insertToSimpleTestTableSQL = R"(
insert into simpletest (first, second) values (?, ?)
)";

static const char* s_selectFromSimpleTestTableSQL = R"(
select first, second from simpletest
)";

void CreateSimpleTestTable(Connection& connection)
{
    Statement createTable = Statement::Create(connection, s_CreateSimpleTestTableSQL);
    REQUIRE_FALSE(createTable.Step());
    REQUIRE(createTable.GetState() == Statement::State::Completed);
}

void InsertIntoSimpleTestTable(Connection& connection, int firstVal, const std::string& secondVal)
{
    Statement insert = Statement::Create(connection, s_insertToSimpleTestTableSQL);

    insert.Bind(1, firstVal);
    insert.Bind(2, secondVal);

    REQUIRE_FALSE(insert.Step());
    REQUIRE(insert.GetState() == Statement::State::Completed);
}

void SelectFromSimpleTestTableOnlyOneRow(Connection& connection, int firstVal, const std::string& secondVal)
{
    Builder::StatementBuilder builder;
    builder.Select({ s_firstColumn, s_secondColumn }).From(s_tableName);
    Statement select = builder.Prepare(connection);

    REQUIRE(select.Step());
    REQUIRE(select.GetState() == Statement::State::HasRow);

    int firstRead = select.GetColumn<int>(0);
    std::string secondRead = select.GetColumn<std::string>(1);

    REQUIRE(firstVal == firstRead);
    REQUIRE(secondVal == secondRead);

    auto tuple = select.GetRow<int, std::string>();

    REQUIRE(firstVal == std::get<0>(tuple));
    REQUIRE(secondVal == std::get<1>(tuple));

    REQUIRE_FALSE(select.Step());
    REQUIRE(select.GetState() == Statement::State::Completed);

    select.Reset();
    REQUIRE(select.GetState() == Statement::State::Prepared);

    REQUIRE(select.Step());
    REQUIRE(select.GetState() == Statement::State::HasRow);
}

TEST_CASE("SQLiteWrapperMemoryCreate", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    int firstVal = 1;
    std::string secondVal = "test";

    InsertIntoSimpleTestTable(connection, firstVal, secondVal);

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLiteWrapperFileCreateAndReopen", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb", ".db" };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int firstVal = 1;
    std::string secondVal = "test";

    // Create the DB and some data
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);

        CreateSimpleTestTable(connection);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    // Reopen the DB and read data
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
    }
}

TEST_CASE("SQLiteWrapperSavepointRollback", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    Savepoint savepoint = Savepoint::Create(connection, "test_savepoint");

    InsertIntoSimpleTestTable(connection, firstVal, secondVal);

    savepoint.Rollback();

    Statement select = Statement::Create(connection, s_selectFromSimpleTestTableSQL);
    REQUIRE(!select.Step());
    REQUIRE(select.GetState() == Statement::State::Completed);
}

TEST_CASE("SQLiteWrapperSavepointRollbackOnDestruct", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    {
        Savepoint savepoint = Savepoint::Create(connection, "test_savepoint");

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    Statement select = Statement::Create(connection, s_selectFromSimpleTestTableSQL);
    REQUIRE(!select.Step());
    REQUIRE(select.GetState() == Statement::State::Completed);
}

TEST_CASE("SQLiteWrapperSavepointCommit", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    {
        Savepoint savepoint = Savepoint::Create(connection, "test_savepoint");

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);

        savepoint.Commit();
    }

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLBuilder_SimpleSelectBind", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    InsertIntoSimpleTestTable(connection, 1, "1");
    InsertIntoSimpleTestTable(connection, 2, "2");
    InsertIntoSimpleTestTable(connection, 3, "3");

    Builder::StatementBuilder builder;
    builder.Select({ s_firstColumn, s_secondColumn }).From(s_tableName).Where(s_firstColumn).Equals(2);

    auto statement = builder.Prepare(connection);

    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 2);
    REQUIRE(statement.GetColumn<std::string>(0) == "2");

    REQUIRE(!statement.Step());
}

TEST_CASE("SQLBuilder_SimpleSelectUnbound", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    InsertIntoSimpleTestTable(connection, 1, "1");
    InsertIntoSimpleTestTable(connection, 2, "2");
    InsertIntoSimpleTestTable(connection, 3, "3");

    Builder::StatementBuilder builder;
    builder.Select({ s_firstColumn, s_secondColumn }).From(s_tableName).Where(s_firstColumn).Equals(Builder::Unbound);

    auto statement = builder.Prepare(connection);

    statement.Bind(1, 2);

    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 2);
    REQUIRE(statement.GetColumn<std::string>(0) == "2");

    REQUIRE(!statement.Step());
}
