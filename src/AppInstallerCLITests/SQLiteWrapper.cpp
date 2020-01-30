// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <SQLiteWrapper.h>

using namespace AppInstaller::Repository::SQLite;

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
    Statement select = Statement::Create(connection, s_selectFromSimpleTestTableSQL);
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
