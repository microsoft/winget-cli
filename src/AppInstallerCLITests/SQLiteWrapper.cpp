// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include <winget/SQLiteMetadataTable.h>
#include <winget/SQLiteVersion.h>

using namespace AppInstaller::SQLite;
using namespace std::string_literals;
using namespace std::string_view_literals;

static const char* s_firstColumn = "first";
static const char* s_secondColumn = "second";
static const char* s_tableName = "simpletest";
static const char* s_savepoint = "simplesave";

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
    Builder::StatementBuilder builder;
    builder.CreateTable(s_tableName).Columns({
        Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int),
        Builder::ColumnBuilder(s_secondColumn, Builder::Type::Text),
        });

    Statement createTable = builder.Prepare(connection);
    REQUIRE_FALSE(createTable.Step());
    REQUIRE(createTable.GetState() == Statement::State::Completed);
}

void InsertIntoSimpleTestTable(Connection& connection, int firstVal, const std::string& secondVal)
{
    Builder::StatementBuilder builder;
    builder.InsertInto(s_tableName).Columns({ s_firstColumn, s_secondColumn }).Values(firstVal, secondVal);
    Statement insert = builder.Prepare(connection);

    REQUIRE_FALSE(insert.Step());
    REQUIRE(insert.GetState() == Statement::State::Completed);
}

void UpdateSimpleTestTable(Connection& connection, int firstVal, const std::string& secondVal)
{
    Builder::StatementBuilder update;
    update.Update(s_tableName).Set().Column(s_firstColumn).Equals(firstVal).Column(s_secondColumn).Equals(secondVal);
    update.Execute(connection);
}

void InsertIntoSimpleTestTableWithNull(Connection& connection, int firstVal)
{
    Builder::StatementBuilder builder;
    builder.InsertInto(s_tableName).Columns({ s_firstColumn, s_secondColumn }).Values(firstVal, nullptr);
    Statement insert = builder.Prepare(connection);

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
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
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

TEST_CASE("SQLiteWrapperSavepointReuse", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int firstVal = 1;
    std::string secondVal = "test";

    // Create the DB and some data
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);

        CreateSimpleTestTable(connection);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    // Reopen the DB and update with a single savepoint
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        Savepoint savepoint = Savepoint::Create(connection, s_savepoint);

        firstVal = 2;
        secondVal = "test2";
        UpdateSimpleTestTable(connection, firstVal, secondVal);
        
        savepoint.Commit();
    }

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
    }

    // Reopen the DB and update with a multiple savepoint
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        {
            Savepoint savepoint = Savepoint::Create(connection, s_savepoint);

            firstVal = 3;
            secondVal = "test3";
            UpdateSimpleTestTable(connection, firstVal, secondVal);
        }

        {
            Savepoint savepoint = Savepoint::Create(connection, s_savepoint);

            firstVal = 4;
            secondVal = "test4";
            UpdateSimpleTestTable(connection, firstVal, secondVal);

            savepoint.Commit();
        }
    }

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
    }
}

TEST_CASE("SQLiteWrapper_EscapeStringForLike", "[sqlitewrapper]")
{
    std::string escape(EscapeCharForLike);

    std::string input = "test";
    std::string output = EscapeStringForLike(input);
    REQUIRE(input == output);

    input = EscapeCharForLike;
    output = EscapeStringForLike(input);
    REQUIRE((input + input) == output);

    input = "%";
    output = EscapeStringForLike(input);
    REQUIRE((escape + input) == output);

    input = "_";
    output = EscapeStringForLike(input);
    REQUIRE((escape + input) == output);

    input = "%_A_%";
    std::string expected = escape + "%" + escape + "_A" + escape + "_" + escape + "%";
    output = EscapeStringForLike(input);
    REQUIRE(expected == output);
}

TEST_CASE("SQLiteWrapper_BindWithEmbeddedNull", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    int firstVal = 1;
    std::string secondVal = "test";
    secondVal[1] = '\0';

    REQUIRE_THROWS_HR(InsertIntoSimpleTestTable(connection, firstVal, secondVal), APPINSTALLER_CLI_ERROR_BIND_WITH_EMBEDDED_NULL);
}

TEST_CASE("SQLiteWrapper_PrepareFailure", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    Builder::StatementBuilder builder;
    builder.Select({ s_firstColumn, s_secondColumn }).From(std::string{ s_tableName } + "2").Where(s_firstColumn).Equals(2);

    REQUIRE_THROWS_HR(builder.Prepare(connection), MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, SQLITE_ERROR));
}

TEST_CASE("SQLiteWrapper_BusyTimeout_None", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    wil::unique_event busy, done;
    busy.create();
    done.create();

    std::thread busyThread([&]()
        {
            Connection threadConnection = Connection::Create(tempFile, Connection::OpenDisposition::Create);
            Statement threadStatement = Statement::Create(threadConnection, "BEGIN EXCLUSIVE TRANSACTION");
            threadStatement.Execute();
            busy.SetEvent();
            done.wait(500);
        });
    busyThread.detach();

    busy.wait(500);

    Connection testConnection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
    testConnection.SetBusyTimeout(0ms);
    Statement testStatement = Statement::Create(testConnection, "BEGIN EXCLUSIVE TRANSACTION");
    REQUIRE_THROWS_HR(testStatement.Execute(), MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, SQLITE_BUSY));

    done.SetEvent();
}

TEST_CASE("SQLiteWrapper_BusyTimeout_Some", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    wil::unique_event busy, ready, done;
    busy.create();
    ready.create();
    done.create();

    std::thread busyThread([&]()
        {
            Connection threadConnection = Connection::Create(tempFile, Connection::OpenDisposition::Create);
            Statement threadBeginStatement = Statement::Create(threadConnection, "BEGIN EXCLUSIVE TRANSACTION");
            Statement threadCommitStatement = Statement::Create(threadConnection, "COMMIT");
            threadBeginStatement.Execute();
            busy.SetEvent();
            ready.wait(500);
            done.wait(100);
            threadCommitStatement.Execute();
        });
    busyThread.detach();

    busy.wait(500);

    Connection testConnection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
    testConnection.SetBusyTimeout(500ms);
    Statement testStatement = Statement::Create(testConnection, "BEGIN EXCLUSIVE TRANSACTION");
    ready.SetEvent();
    testStatement.Execute();

    done.SetEvent();
}

TEST_CASE("SQLiteWrapper_CloseConnectionOnError", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    Builder::StatementBuilder builder;
    builder.CreateTable(s_tableName).Columns({
        Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int),
        Builder::ColumnBuilder(s_secondColumn, Builder::Type::Text),
        });

    Statement createTable = builder.Prepare(connection);
    REQUIRE_FALSE(createTable.Step());
    REQUIRE(createTable.GetState() == Statement::State::Completed);

    createTable.Reset();
    REQUIRE_THROWS(createTable.Step(true));

    // Do anything that needs the connection
    REQUIRE_THROWS_HR(connection.GetLastInsertRowID(), APPINSTALLER_CLI_ERROR_SQLITE_CONNECTION_TERMINATED);
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

    Builder::StatementBuilder buildCount;
    buildCount.Select(Builder::RowCount).From(s_tableName);

    auto rows = buildCount.Prepare(connection);

    REQUIRE(rows.Step());
    REQUIRE(rows.GetColumn<int>(0) == 3);

    REQUIRE(!rows.Step());
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

TEST_CASE("SQLBuilder_SimpleSelectNull", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    InsertIntoSimpleTestTable(connection, 1, "1");
    InsertIntoSimpleTestTable(connection, 2, "2");
    InsertIntoSimpleTestTableWithNull(connection, 3);

    Builder::StatementBuilder builder;
    builder.Select({ s_firstColumn, s_secondColumn }).From(s_tableName).Where(s_secondColumn).IsNull();

    auto statement = builder.Prepare(connection);

    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 3);
    REQUIRE(statement.GetColumnIsNull(1));

    REQUIRE(!statement.Step());
}

TEST_CASE("SQLBuilder_SimpleSelectOptional", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    InsertIntoSimpleTestTable(connection, 1, "1");
    InsertIntoSimpleTestTable(connection, 2, "2");
    InsertIntoSimpleTestTableWithNull(connection, 3);

    std::optional<std::string> secondValue;

    {
        Builder::StatementBuilder builder;
        builder.Select({ s_firstColumn, s_secondColumn }).From(s_tableName).Where(s_secondColumn).Equals(secondValue);

        auto statement = builder.Prepare(connection);

        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 3);
        REQUIRE(statement.GetColumnIsNull(1));

        REQUIRE(!statement.Step());
    }

    {
        secondValue = "2";
        Builder::StatementBuilder builder;
        builder.Select({ s_firstColumn, s_secondColumn }).From(s_tableName).Where(s_secondColumn).Equals(secondValue);

        auto statement = builder.Prepare(connection);

        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 2);
        REQUIRE(statement.GetColumn<std::string>(1) == "2");

        REQUIRE(!statement.Step());
    }
}

TEST_CASE("SQLBuilder_Update", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);

    int firstVal = 1;
    std::string secondVal = "test";

    InsertIntoSimpleTestTable(connection, firstVal, secondVal);

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);

    firstVal = 2;
    secondVal = "testing";

    UpdateSimpleTestTable(connection, firstVal, secondVal);

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLBuilder_CaseInsensitive", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    Builder::StatementBuilder createTable;
    createTable.CreateTable(s_tableName).Columns({
        Builder::ColumnBuilder(s_firstColumn, Builder::Type::Text).CollateNoCase()
        });

    createTable.Execute(connection);

    std::string upperCaseVal = "TEST";
    std::string lowerCaseVal = "test";

    {
        INFO("Insert initial value");
        Builder::StatementBuilder builder;
        builder.InsertInto(s_tableName)
            .Columns({ s_firstColumn })
            .Values(upperCaseVal);

        builder.Execute(connection);
    }

    {
        INFO("Retrieve using case-insensitive value");
        Builder::StatementBuilder builder;
        builder.Select({ s_firstColumn }).From(s_tableName).Where(s_firstColumn).Equals(lowerCaseVal);

        auto statement = builder.Prepare(connection);
        REQUIRE(statement.Step());
    }
}

TEST_CASE("SQLBuilder_CreateTable", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int testRun = GENERATE(0, 1, 2, 3, 4, 5, 6, 7);

    bool notNull = ((testRun & 1) != 0);
    bool unique = ((testRun & 2) != 0);
    bool pk = ((testRun & 4) != 0);
    CAPTURE(notNull, unique, pk);

    Builder::StatementBuilder createTable;
    createTable.CreateTable(s_tableName).Columns({
        Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int).NotNull(notNull).Unique(unique).PrimaryKey(pk)
        });

    createTable.Execute(connection);

    Builder::StatementBuilder insertBuilder;
    insertBuilder.InsertInto(s_tableName).Columns(s_firstColumn).Values(Builder::Unbound);

    Statement insertStatement = insertBuilder.Prepare(connection);

    {
        INFO("Insert NULL");
        insertStatement.Bind(1, nullptr);

        if (notNull)
        {
            REQUIRE_THROWS_HR(insertStatement.Execute(), MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, SQLITE_CONSTRAINT_NOTNULL));
        }
        else
        {
            insertStatement.Execute();
        }
    }

    {
        INFO("Insert unique values");
        insertStatement.Reset();
        insertStatement.Bind(1, 1);
        insertStatement.Execute();

        insertStatement.Reset();
        insertStatement.Bind(1, 2);
        insertStatement.Execute();
    }

    {
        INFO("Insert duplicate values");
        insertStatement.Reset();
        insertStatement.Bind(1, 1);

        if (unique || pk)
        {
            HRESULT expectedHR = S_OK;
            if (pk)
            {
                expectedHR = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, SQLITE_CONSTRAINT_PRIMARYKEY);
            }
            else
            {
                expectedHR = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, SQLITE_CONSTRAINT_UNIQUE);
            }
            REQUIRE_THROWS_HR(insertStatement.Execute(), expectedHR);
        }
        else
        {
            insertStatement.Execute();
        }
    }
}

TEST_CASE("SQLBuilder_InsertValueBinding", "[sqlbuilder]")
{
    char const* const columns[] = { "a", "b", "c", "d", "e", "f" };

    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);

    {
        INFO("Create table");
        Builder::StatementBuilder createTable;
        createTable.CreateTable(s_tableName).BeginColumns();
        for (const auto c : columns)
        {
            createTable.Column(Builder::ColumnBuilder(c, Builder::Type::Int));
        }
        createTable.EndColumns();
        createTable.Execute(connection);
    }

    {
        INFO("Insert values");
        Builder::StatementBuilder insertBuilder;
        insertBuilder.InsertInto(s_tableName).BeginColumns();
        for (const auto c : columns)
        {
            insertBuilder.Column(c);
        }
        insertBuilder.EndColumns().Values(0, 1, 2, 3, 4, 5);
        insertBuilder.Execute(connection);
    }

    {
        INFO("Insert values");
        Builder::StatementBuilder insertBuilder;
        insertBuilder.InsertInto(s_tableName).BeginColumns();
        for (const auto c : columns)
        {
            insertBuilder.Column(c);
        }
        insertBuilder.EndColumns().BeginValues();
        insertBuilder.Value(5);
        insertBuilder.Value(nullptr);
        insertBuilder.Value(3);
        insertBuilder.Value(std::optional<int>{});
        insertBuilder.Value(std::optional<int>{ 1 });
        insertBuilder.Value(Builder::Unbound);
        insertBuilder.EndValues();
        insertBuilder.Execute(connection);
    }

    {
        INFO("Select values");
        Builder::StatementBuilder selectBuilder;
        selectBuilder.Select();
        for (const auto c : columns)
        {
            selectBuilder.Column(c);
        }
        selectBuilder.From(s_tableName);

        Statement select = selectBuilder.Prepare(connection);
        REQUIRE(select.Step());

        for (int i = 0; i < ARRAYSIZE(columns); ++i)
        {
            REQUIRE(i == select.GetColumn<int>(i));
        }

        REQUIRE(select.Step());

        for (int i = 0; i < ARRAYSIZE(columns); ++i)
        {
            if (i & 1)
            {
                REQUIRE(select.GetColumnIsNull(i));
            }
            else
            {
                REQUIRE((5 - i) == select.GetColumn<int>(i));
            }
        }

        REQUIRE(!select.Step());
    }
}

TEST_CASE("SQLBuilder_AssignValueNull", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);
    InsertIntoSimpleTestTable(connection, 1, "value");

    {
        INFO("Equals(nullptr) remains blocked as a filter");
        Builder::StatementBuilder builder;
        REQUIRE_THROWS_HR(builder.Select(s_firstColumn).From(s_tableName).Where(s_secondColumn).Equals(nullptr), E_NOTIMPL);
    }

    {
        INFO("AssignValue(nullptr) assigns NULL in an update");
        Builder::StatementBuilder update;
        update.Update(s_tableName).Set().Column(s_secondColumn).AssignValue(nullptr).Where(s_firstColumn).Equals(1);
        update.Execute(connection);
    }

    {
        INFO("The value is now NULL");
        Builder::StatementBuilder select;
        select.Select({ s_firstColumn, s_secondColumn }).From(s_tableName);

        Statement statement = select.Prepare(connection);
        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 1);
        REQUIRE(statement.GetColumnIsNull(1));
        REQUIRE(!statement.Step());
    }
}

TEST_CASE("SQLBuilder_AddColumnWithConstraints", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);
    InsertIntoSimpleTestTable(connection, 1, "one");

    constexpr std::string_view addedColumn = "added";

    {
        // SQLite requires a non-null default when adding a column declared as not null,
        // so the plain Add(column, type) form cannot express this.
        INFO("Add a not null column with a default");
        Builder::StatementBuilder alter;
        alter.AlterTable(s_tableName).Add(Builder::ColumnBuilder(addedColumn, Builder::Type::Int64).NotNull().Default(0));
        alter.Execute(connection);
    }

    {
        INFO("The existing row receives the default rather than null");
        Builder::StatementBuilder select;
        select.Select(addedColumn).From(s_tableName);

        Statement statement = select.Prepare(connection);
        REQUIRE(statement.Step());
        REQUIRE(!statement.GetColumnIsNull(0));
        REQUIRE(statement.GetColumn<int64_t>(0) == 0);
        REQUIRE(!statement.Step());
    }
}

TEST_CASE("SQLBuilder_CreateTempView", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);
    InsertIntoSimpleTestTable(connection, 1, "one");
    InsertIntoSimpleTestTable(connection, 2, "two");

    constexpr std::string_view viewName = "simple_view";

    {
        // Note that SQLite prohibits bound parameters in a view definition, so the
        // statement that defines a view must be structural only.
        INFO("Create a view over the table");
        Builder::StatementBuilder createView;
        createView.CreateTempView(viewName).Select({ s_firstColumn, s_secondColumn }).From(s_tableName).OrderBy(s_firstColumn);
        createView.Execute(connection);
    }

    {
        INFO("The view returns the underlying rows");
        Builder::StatementBuilder select;
        select.Select({ s_firstColumn, s_secondColumn }).From(viewName);

        Statement statement = select.Prepare(connection);
        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 1);
        REQUIRE(statement.GetColumn<std::string>(1) == "one");
        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 2);
        REQUIRE(statement.GetColumn<std::string>(1) == "two");
        REQUIRE(!statement.Step());
    }

    {
        INFO("A filter can still be applied when reading the view");
        Builder::StatementBuilder select;
        select.Select(s_secondColumn).From(viewName).Where(s_firstColumn).Equals(2);

        Statement statement = select.Prepare(connection);
        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<std::string>(0) == "two");
        REQUIRE(!statement.Step());
    }
}

TEST_CASE("SQLBuilder_UnionAll", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    CreateSimpleTestTable(connection);
    InsertIntoSimpleTestTable(connection, 1, "one");
    InsertIntoSimpleTestTable(connection, 2, "two");

    Builder::StatementBuilder select;
    select.Select(s_firstColumn).From(s_tableName).Where(s_firstColumn).Equals(1).
        UnionAll().
        Select(s_firstColumn).From(s_tableName).Where(s_firstColumn).Equals(2);

    Statement statement = select.Prepare(connection);

    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 1);
    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 2);
    REQUIRE(!statement.Step());
}

TEST_CASE("SQLBuilder_NotExists", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    constexpr std::string_view otherTable = "other_test";

    CreateSimpleTestTable(connection);
    InsertIntoSimpleTestTable(connection, 1, "one");
    InsertIntoSimpleTestTable(connection, 2, "two");

    {
        Builder::StatementBuilder createTable;
        createTable.CreateTable(otherTable).Columns({ Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int) });
        createTable.Execute(connection);

        Builder::StatementBuilder insert;
        insert.InsertInto(otherTable).Columns(s_firstColumn).Values(2);
        insert.Execute(connection);
    }

    // Select rows from the simple table that have no matching row in the other table.
    Builder::StatementBuilder select;
    select.Select(Builder::QualifiedColumn{ s_tableName, s_firstColumn }).From(s_tableName).
        Where().NotExists().BeginParenthetical().
            Select(Builder::QualifiedColumn{ otherTable, s_firstColumn }).From(otherTable).
            Where(Builder::QualifiedColumn{ otherTable, s_firstColumn }).Equals(Builder::QualifiedColumn{ s_tableName, s_firstColumn }).
        EndParenthetical();

    Statement statement = select.Prepare(connection);

    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 1);
    REQUIRE(!statement.Step());
}

TEST_CASE("SQLBuilder_AttachAndTempView", "[sqlbuilder]")
{
    TestCommon::TempFile baselineFile{ "repolibtest_baseline"s, ".db"s };
    INFO("Using temporary file named: " << baselineFile.GetPath());

    {
        INFO("Create the database that will be attached");
        Connection baseline = Connection::Create(baselineFile, Connection::OpenDisposition::Create);
        CreateSimpleTestTable(baseline);
        InsertIntoSimpleTestTable(baseline, 1, "baseline");
    }

    // The host is created rather than named through a specifier, which is the case that proves URI
    // handling is a property of the connection: SQLite decides whether names are URIs when the
    // connection is opened and applies that to every later ATTACH, so a host that did not ask for
    // it would take the attached database's URI as a literal filename.
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    constexpr std::string_view baselineAlias = "baseline";
    constexpr std::string_view deltaTable = "delta_test";

    {
        INFO("Create a local table with a distinct row");
        Builder::StatementBuilder createTable;
        createTable.CreateTable(deltaTable).Columns({
            Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int),
            Builder::ColumnBuilder(s_secondColumn, Builder::Type::Text),
            });
        createTable.Execute(connection);

        Builder::StatementBuilder insert;
        insert.InsertInto(deltaTable).Columns({ s_firstColumn, s_secondColumn }).Values(2, "delta"sv);
        insert.Execute(connection);
    }

    {
        INFO("Attach the baseline database");
        Builder::StatementBuilder attach;
        attach.Attach(DatabaseSpecifier{ baselineFile.GetPath().u8string(), DatabaseDisposition::Read }, baselineAlias);
        attach.Execute(connection);
    }

    {
        INFO("A temp view can span the local and attached databases");
        Builder::StatementBuilder createView;
        createView.CreateTempView(s_tableName).
            Select({ s_firstColumn, s_secondColumn }).From(deltaTable).
            UnionAll().
            Select({ s_firstColumn, s_secondColumn }).From(Builder::QualifiedTable{ baselineAlias, s_tableName });
        createView.Execute(connection);
    }

    {
        INFO("Reading the view returns the merged rows");
        Builder::StatementBuilder select;
        select.Select({ s_firstColumn, s_secondColumn }).From(s_tableName).OrderBy(s_firstColumn);

        Statement statement = select.Prepare(connection);

        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 1);
        REQUIRE(statement.GetColumn<std::string>(1) == "baseline");

        REQUIRE(statement.Step());
        REQUIRE(statement.GetColumn<int>(0) == 2);
        REQUIRE(statement.GetColumn<std::string>(1) == "delta");

        REQUIRE(!statement.Step());
    }
}

// ATTACH takes no flags of its own: it starts from the ones the connection was opened with. A
// database named by a plain path would therefore be attached read/write whenever its host is, so
// the disposition has to travel in the name.
TEST_CASE("SQLBuilder_AttachHonorsDisposition", "[sqlbuilder]")
{
    TestCommon::TempFile mainFile{ "repolibtest_attach_main"s, ".db"s };
    TestCommon::TempFile attachedFile{ "repolibtest_attach_readonly"s, ".db"s };

    {
        Connection main = Connection::Create(mainFile, Connection::OpenDisposition::Create);
        CreateSimpleTestTable(main);
    }

    {
        Connection attached = Connection::Create(attachedFile, Connection::OpenDisposition::Create);
        CreateSimpleTestTable(attached);
        InsertIntoSimpleTestTable(attached, 1, "attached");
    }

    constexpr std::string_view alias = "other";

    // The host connection can write, which is what makes the attachment worth asserting.
    Connection connection = Connection::Create(DatabaseSpecifier{ mainFile.GetPath().u8string(), DatabaseDisposition::ReadWrite });

    {
        Builder::StatementBuilder attach;
        attach.Attach(DatabaseSpecifier{ attachedFile.GetPath().u8string(), DatabaseDisposition::Read }, alias);
        attach.Execute(connection);
    }

    {
        INFO("The primary database is writable");
        Builder::StatementBuilder insert;
        insert.InsertInto(s_tableName).Columns({ s_firstColumn, s_secondColumn }).Values(2, "main"sv);
        REQUIRE_NOTHROW(insert.Execute(connection));
    }

    {
        INFO("The attached database is not");
        Builder::StatementBuilder insert;
        insert.InsertInto(Builder::QualifiedTable{ alias, s_tableName }).Columns({ s_firstColumn, s_secondColumn }).Values(3, "attached"sv);
        REQUIRE_THROWS(insert.Execute(connection));
    }
}

// Reading metadata from an attached database is what lets a caller validate the database it is
// actually going to read, rather than a separate connection's view of the same path.
TEST_CASE("SQLiteMetadata_AttachedDatabase", "[sqlitewrapper]")
{
    TestCommon::TempFile attachedFile{ "repolibtest_metadata_attached"s, ".db"s };
    INFO("Using temporary file named: " << attachedFile.GetPath());

    {
        Connection attached = Connection::Create(attachedFile, Connection::OpenDisposition::Create);
        MetadataTable::Create(attached);
        MetadataTable::SetNamedValue(attached, s_MetadataValueName_MajorVersion, 2);
        MetadataTable::SetNamedValue(attached, s_MetadataValueName_MinorVersion, 1);
        MetadataTable::SetNamedValue(attached, "shared"sv, "attachedValue"s);
        MetadataTable::SetNamedValue(attached, "onlyAttached"sv, "present"s);
    }

    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);
    MetadataTable::Create(connection);
    MetadataTable::SetNamedValue(connection, s_MetadataValueName_MajorVersion, 3);
    MetadataTable::SetNamedValue(connection, s_MetadataValueName_MinorVersion, 4);
    MetadataTable::SetNamedValue(connection, "shared"sv, "mainValue"s);

    constexpr std::string_view alias = "other";

    {
        Builder::StatementBuilder attach;
        attach.Attach(DatabaseSpecifier{ attachedFile.GetPath().u8string(), DatabaseDisposition::Read }, alias);
        attach.Execute(connection);
    }

    // Both databases have a metadata table holding the same name, so an unqualified read would
    // silently answer from the wrong one.
    REQUIRE(MetadataTable::GetNamedValue<std::string>(connection, "shared"sv) == "mainValue");
    REQUIRE(MetadataTable::GetNamedValue<std::string>(connection, "shared"sv, alias) == "attachedValue");

    // A value only the attachment has is unreachable without targeting it.
    REQUIRE(!MetadataTable::TryGetNamedValue<std::string>(connection, "onlyAttached"sv).has_value());
    REQUIRE(MetadataTable::TryGetNamedValue<std::string>(connection, "onlyAttached"sv, alias) == "present"s);

    // Absence is still reported as absence rather than falling back to the primary database.
    REQUIRE(!MetadataTable::TryGetNamedValue<std::string>(connection, "onlyMain"sv, alias).has_value());

    REQUIRE(Version::GetSchemaVersion(connection) == Version{ 3, 4 });
    REQUIRE(Version::GetSchemaVersion(connection, alias) == Version{ 2, 1 });
}

// Detaching has to actually release the alias, or a caller that rejects one database cannot try
// another on the same connection.
TEST_CASE("SQLBuilder_Detach", "[sqlbuilder]")
{
    TestCommon::TempFile firstFile{ "repolibtest_detach_first"s, ".db"s };
    TestCommon::TempFile secondFile{ "repolibtest_detach_second"s, ".db"s };

    constexpr std::string_view firstMarker = "first";
    constexpr std::string_view secondMarker = "second";

    auto seed = [](const TestCommon::TempFile& file, std::string_view marker)
        {
            Connection attached = Connection::Create(file, Connection::OpenDisposition::Create);
            MetadataTable::Create(attached);
            MetadataTable::SetNamedValue(attached, "which"sv, std::string{ marker });
        };

    seed(firstFile, firstMarker);
    seed(secondFile, secondMarker);

    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    constexpr std::string_view alias = "attached";

    auto attach = [&](const TestCommon::TempFile& file)
        {
            Builder::StatementBuilder builder;
            builder.Attach(DatabaseSpecifier{ file.GetPath().u8string(), DatabaseDisposition::Read }, alias);
            builder.Execute(connection);
        };

    attach(firstFile);
    REQUIRE(MetadataTable::GetNamedValue<std::string>(connection, "which"sv, alias) == firstMarker);

    // Reusing an alias that is still in use is an error, which is what makes releasing it matter.
    REQUIRE_THROWS(attach(secondFile));

    {
        Builder::StatementBuilder detach;
        detach.Detach(alias);
        detach.Execute(connection);
    }

    attach(secondFile);
    REQUIRE(MetadataTable::GetNamedValue<std::string>(connection, "which"sv, alias) == secondMarker);
}

TEST_CASE("SQLBuilder_ViewWithTombstoneSuppression", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    constexpr std::string_view valueTombstoneTable = "value_tombstone";
    constexpr std::string_view ownerTombstoneTable = "owner_tombstone";
    constexpr std::string_view removedColumn = "is_removed";
    constexpr std::string_view valueAlias = "v";
    constexpr std::string_view ownerAlias = "o";
    constexpr std::string_view viewName = "survivors";

    CreateSimpleTestTable(connection);
    InsertIntoSimpleTestTable(connection, 1, "kept");
    InsertIntoSimpleTestTable(connection, 2, "value removed");
    InsertIntoSimpleTestTable(connection, 3, "owner removed");

    auto createTombstoneTable = [&](std::string_view tableName, int suppressedValue)
        {
            Builder::StatementBuilder createTable;
            createTable.CreateTable(tableName).Columns({
                Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int),
                Builder::ColumnBuilder(removedColumn, Builder::Type::Int),
                });
            createTable.Execute(connection);

            Builder::StatementBuilder insertSuppressed;
            insertSuppressed.InsertInto(tableName).Columns({ s_firstColumn, removedColumn }).Values(suppressedValue, 1);
            insertSuppressed.Execute(connection);

            // Row 1 is named by both tables without being removed by either, which is what
            // distinguishes a test of the removal flag from a test of mere presence.
            Builder::StatementBuilder insertMentioned;
            insertMentioned.InsertInto(tableName).Columns({ s_firstColumn, removedColumn }).Values(1, 0);
            insertMentioned.Execute(connection);
        };

    createTombstoneTable(valueTombstoneTable, 2);
    createTombstoneTable(ownerTombstoneTable, 3);

    {
        INFO("A view cannot contain bound parameters, so its comparisons have to be literals");
        Builder::StatementBuilder createView;
        createView.CreateTempView(viewName).
            Select({ s_firstColumn, s_secondColumn }).From(s_tableName).
            Where().NotExists().BeginParenthetical().
                Select(s_firstColumn).From(valueTombstoneTable).As(valueAlias).
                Where(Builder::QualifiedColumn{ valueAlias, s_firstColumn }).Equals(Builder::QualifiedColumn{ s_tableName, s_firstColumn }).
                And(Builder::QualifiedColumn{ valueAlias, removedColumn }).EqualsLiteral(1).
            EndParenthetical().
            And().NotExists().BeginParenthetical().
                Select(s_firstColumn).From(ownerTombstoneTable).As(ownerAlias).
                Where(Builder::QualifiedColumn{ ownerAlias, s_firstColumn }).Equals(Builder::QualifiedColumn{ s_tableName, s_firstColumn }).
                And(Builder::QualifiedColumn{ ownerAlias, removedColumn }).EqualsLiteral(1).
            EndParenthetical();
        createView.Execute(connection);
    }

    INFO("Only the row that neither tombstone removes survives");
    Builder::StatementBuilder select;
    select.Select({ s_firstColumn, s_secondColumn }).From(viewName).OrderBy(s_firstColumn);

    Statement statement = select.Prepare(connection);

    REQUIRE(statement.Step());
    REQUIRE(statement.GetColumn<int>(0) == 1);
    REQUIRE(statement.GetColumn<std::string>(1) == "kept");

    REQUIRE(!statement.Step());
}

TEST_CASE("SQLBuilder_NotEqualsLiteral", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    constexpr std::string_view flagTableName = "flagged";
    constexpr std::string_view removedColumn = "is_removed";
    constexpr std::string_view liveView = "live";
    constexpr std::string_view removedView = "removed";

    {
        Builder::StatementBuilder createTable;
        createTable.CreateTable(flagTableName).Columns({
            Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int),
            Builder::ColumnBuilder(removedColumn, Builder::Type::Int),
            });
        createTable.Execute(connection);
    }

    auto insert = [&](int value, int removed)
        {
            Builder::StatementBuilder builder;
            builder.InsertInto(flagTableName).Columns({ s_firstColumn, removedColumn }).Values(value, removed);
            builder.Execute(connection);
        };

    insert(1, 0);
    insert(2, 1);

    // Nothing constrains the column to 0 and 1. This is the value that a complementary pair of
    // equality tests would place in neither class, which is the state the negated form removes.
    insert(3, 2);

    auto createView = [&](std::string_view viewName, bool live)
        {
            INFO("A view cannot contain bound parameters, so its comparisons have to be literals");
            Builder::StatementBuilder builder;
            builder.CreateTempView(viewName).Select(s_firstColumn).From(flagTableName).Where(removedColumn);

            if (live)
            {
                builder.EqualsLiteral(0);
            }
            else
            {
                builder.NotEqualsLiteral(0);
            }

            builder.Execute(connection);
        };

    createView(liveView, true);
    createView(removedView, false);

    auto readView = [&](std::string_view viewName)
        {
            Builder::StatementBuilder builder;
            builder.Select(s_firstColumn).From(viewName).OrderBy(s_firstColumn);

            Statement statement = builder.Prepare(connection);

            std::vector<int> result;
            while (statement.Step())
            {
                result.emplace_back(statement.GetColumn<int>(0));
            }

            return result;
        };

    INFO("The two predicates partition the table, leaving no row in neither");
    REQUIRE(readView(liveView) == std::vector<int>{ 1 });
    REQUIRE(readView(removedView) == std::vector<int>{ 2, 3 });
}

TEST_CASE("SQLBuilder_SchemaTableExists", "[sqlbuilder]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    constexpr std::string_view tableName = "present";
    constexpr std::string_view indexName = "index_only";
    constexpr std::string_view viewName = "view_only";

    REQUIRE(!Builder::Schema::TableExists(connection, tableName));

    {
        Builder::StatementBuilder builder;
        builder.CreateTable(tableName).Columns({ Builder::ColumnBuilder(s_firstColumn, Builder::Type::Int) });
        builder.Execute(connection);
    }

    REQUIRE(Builder::Schema::TableExists(connection, tableName));

    {
        Builder::StatementBuilder builder;
        builder.CreateIndex(indexName).On(tableName).Columns(s_firstColumn);
        builder.Execute(connection);
    }

    {
        Builder::StatementBuilder builder;
        builder.CreateTempView(viewName).Select(s_firstColumn).From(tableName);
        builder.Execute(connection);
    }

    INFO("An index lives in the same schema table as its table, so only the type filter keeps it out");
    REQUIRE(!Builder::Schema::TableExists(connection, indexName));

    INFO("A temp view is not in the main schema at all. The delta read form gives its merged views "
        "the names of real 2.0 tables, so this is why PackagesTable::Exists reports false there");
    REQUIRE(!Builder::Schema::TableExists(connection, viewName));
}

TEST_CASE("SQLiteWrapperTransactionRollback", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    Transaction transaction = Transaction::Create(connection, "test_transaction", false);

    InsertIntoSimpleTestTable(connection, firstVal, secondVal);

    transaction.Rollback();

    Statement select = Statement::Create(connection, s_selectFromSimpleTestTableSQL);
    REQUIRE(!select.Step());
    REQUIRE(select.GetState() == Statement::State::Completed);
}

TEST_CASE("SQLiteWrapperTransactionRollbackOnDestruct", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    {
        Transaction transaction = Transaction::Create(connection, "test_transaction", false);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    Statement select = Statement::Create(connection, s_selectFromSimpleTestTableSQL);
    REQUIRE(!select.Step());
    REQUIRE(select.GetState() == Statement::State::Completed);
}

TEST_CASE("SQLiteWrapperTransactionCommit", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    {
        Transaction transaction = Transaction::Create(connection, "test_transaction", false);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);

        transaction.Commit();
    }

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLiteWrapperTransactionImmediate", "[sqlitewrapper]")
{
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    {
        Transaction transaction = Transaction::Create(connection, "test_transaction", true);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);

        transaction.Commit();
    }

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLiteWrapperTransactionWriteConflict", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);
    connection.SetJournalMode("WAL");

    int firstVal = 1;
    std::string secondVal = "test";

    CreateSimpleTestTable(connection);

    Connection connection2 = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
    std::chrono::milliseconds busyWait = 250ms;
    connection2.SetBusyTimeout(busyWait);

    {
        Transaction transaction = Transaction::Create(connection, "test_transaction", true);
        InsertIntoSimpleTestTable(connection, firstVal, secondVal);

        // Start second transaction
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point end = start;
        try
        {
            Transaction transaction2 = Transaction::Create(connection2, "test_transaction2", true);
        }
        catch (...)
        {
            end = std::chrono::system_clock::now();
        }

        std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        REQUIRE(duration >= busyWait);

        transaction.Commit();

        Transaction transaction2 = Transaction::Create(connection2, "test_transaction2", true);
        InsertIntoSimpleTestTable(connection2, firstVal, secondVal);
    }

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLiteDatabaseSpecifierTargets", "[sqlitewrapper]")
{
    // Every disposition is carried as a URI query parameter, because ATTACH takes no flags of its
    // own and would otherwise give the attached database whatever access its host connection has.
    DatabaseSpecifier read{ "D:\\test\\index.db"s, DatabaseDisposition::Read };
    REQUIRE(read.Target() == "file:/D:/test/index.db?mode=ro");
    REQUIRE(read.ConnectionDisposition() == Connection::OpenDisposition::ReadOnly);

    DatabaseSpecifier readWrite{ "D:\\test\\index.db"s, DatabaseDisposition::ReadWrite };
    REQUIRE(readWrite.Target() == "file:/D:/test/index.db?mode=rw");
    REQUIRE(readWrite.ConnectionDisposition() == Connection::OpenDisposition::ReadWrite);

    // Immutability is not something that the mode parameter can express.
    DatabaseSpecifier immutable{ "D:\\test\\index.db"s, DatabaseDisposition::Immutable };
    REQUIRE(immutable.Target() == "file:/D:/test/index.db?immutable=1");
    REQUIRE(immutable.ConnectionDisposition() == Connection::OpenDisposition::ReadOnly);

    // Characters that would otherwise start the query or fragment are escaped, and repeated
    // separators collapse, per the conversion the URI documentation prescribes.
    DatabaseSpecifier escaped{ "D:\\a#b\\\\c?d\\index.db"s, DatabaseDisposition::Immutable };
    REQUIRE(escaped.Target() == "file:/D:/a%23b/c%3fd/index.db?immutable=1");

    // A UNC path is the one case where the leading separators must not collapse: the pair has to
    // survive, which takes an empty authority ahead of it. Anything else names the server as the
    // authority, which SQLite rejects.
    DatabaseSpecifier unc{ "\\\\server\\share\\index.db"s, DatabaseDisposition::Read };
    REQUIRE(unc.Target() == "file:////server/share/index.db?mode=ro");

    // A percent is a legal filename character, but SQLite decodes %HH escapes out of the path. Left
    // alone, this name would be read as index#.db -- a different file, if it exists at all.
    DatabaseSpecifier percent{ "D:\\test\\index%23.db"s, DatabaseDisposition::Read };
    REQUIRE(percent.Target() == "file:/D:/test/index%2523.db?mode=ro");
}

// The escaping is only worth anything if a file so named can actually be opened, which is the part
// no amount of string comparison can establish.
TEST_CASE("SQLiteDatabaseSpecifierEscapedPathOpen", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_temp%23db"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int firstVal = 1;
    std::string secondVal = "test";

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);
        CreateSimpleTestTable(connection);
        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    DatabaseSpecifier specifier{ tempFile.GetPath().u8string(), DatabaseDisposition::Read };
    Connection connection = Connection::Create(specifier);

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

// The administrative share is used rather than creating one, so this reaches the same local file by
// a UNC name without changing the machine. It is not reachable without elevation, so the test
// yields instead of failing when it is absent.
TEST_CASE("SQLiteDatabaseSpecifierUncOpen", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::wstring localPath = tempFile.GetPath().wstring();

    if (localPath.size() < 3 || localPath[1] != L':' || localPath[2] != L'\\')
    {
        WARN("Temporary file is not named by a drive letter; skipping UNC coverage");
        return;
    }

    // C:\dir\file.db -> \\localhost\C$\dir\file.db
    std::filesystem::path uncPath{ L"\\\\localhost\\" + localPath.substr(0, 1) + L"$" + localPath.substr(2) };
    INFO("Using UNC name: " << uncPath);

    int firstVal = 1;
    std::string secondVal = "test";

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);
        CreateSimpleTestTable(connection);
        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    if (!std::filesystem::exists(uncPath))
    {
        WARN("Administrative share is not reachable; skipping UNC coverage");
        return;
    }

    DatabaseSpecifier specifier{ uncPath.u8string(), DatabaseDisposition::Read };

    std::string expectedPrefix = "file:////localhost/" + std::string{ static_cast<char>(localPath[0]) } + "$/";
    REQUIRE(specifier.Target().substr(0, expectedPrefix.size()) == expectedPrefix);

    Connection connection = Connection::Create(specifier);

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}

TEST_CASE("SQLiteDatabaseSpecifierImmutableOpen", "[sqlitewrapper]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int firstVal = 1;
    std::string secondVal = "test";

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);

        CreateSimpleTestTable(connection);

        InsertIntoSimpleTestTable(connection, firstVal, secondVal);
    }

    Connection connection = Connection::Create(DatabaseSpecifier{ tempFile.GetPath().u8string(), DatabaseDisposition::Immutable });

    SelectFromSimpleTestTableOnlyOneRow(connection, firstVal, secondVal);
}
