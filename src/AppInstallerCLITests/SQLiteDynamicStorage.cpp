// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerErrors.h>
#include <winget/SQLiteDynamicStorage.h>
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include <winget/SQLiteVersion.h>

using namespace AppInstaller::SQLite;
using namespace std::string_literals;

TEST_CASE("SQLiteDynamicStorage_UpgradeDetection", "[sqlite_dynamic]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Create a database with version 1.0
    SQLiteDynamicStorage storage{ tempFile.GetPath(), Version{ 1, 0 } };

    {
        auto transactionLock = storage.TryBeginTransaction("test", false);
        REQUIRE(transactionLock);
    }

    // Update the database to version 2.0
    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::Create);
        Version version{ 2, 0 };
        version.SetSchemaVersion(connection);
    }

    REQUIRE(storage.GetVersion() == Version{ 1, 0 });

    auto transactionLock = storage.TryBeginTransaction("test", false);
    REQUIRE(!transactionLock);

    REQUIRE(storage.GetVersion() == Version{ 2, 0 });

    transactionLock = storage.TryBeginTransaction("test", false);
    REQUIRE(transactionLock);
}
