// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <SQLiteWrapper.h>

using namespace AppInstaller::Repository::SQLite;

#define SQLITE_MEMORY_DB_CONNECTION_TARGET ":memory:"

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
    char tempPath[MAX_PATH]{};
    REQUIRE(GetTempPathA(MAX_PATH, tempPath) != 0);

    srand(static_cast<unsigned int>(time(NULL)));
    std::stringstream tempFileName;
    tempFileName << tempPath << "\\repolibtest_tempdb" << rand() << ".db";

    INFO("Using temporary file named: " << tempFileName.str());

    DeleteFileA(tempFileName.str().c_str());

    int firstVal = 1;
    std::string secondVal = "test";

    // Create the DB and some data
    {
        Connection connection = Connection::Create(tempFileName.str(), Connection::OpenDisposition::Create);

        CreateSimpleTestTable(connection);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    // Reopen the DB and read data
    {
        Connection connection = Connection::Create(tempFileName.str(), Connection::OpenDisposition::ReadWrite);

        SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
    }
}
