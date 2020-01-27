// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <SQLiteWrapper.h>
#include <Manifest/Manifest.h>
#include <Microsoft/SQLiteIndex.h>
#include <Microsoft/Schema/1_0/PathPartTable.h>

using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;

TEST_CASE("SQLiteIndexCreateLatestAndReopen", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb", ".db" };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Schema::Version versionCreated;

    // Create the index
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index for read only
    {
        INFO("Trying with Read");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::Read);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::Immutable);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("SQLiteIndexCreateAndAddManifest", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb", ".db" };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());

    Manifest manifest;
    manifest.Id = "test.id";
    manifest.Name = "Test Name";
    manifest.AppMoniker = "testmoniker";
    manifest.Version = "1.0.0";
    manifest.Channel = "test";
    manifest.Tags = { "t1", "t2" };
    manifest.Commands = { "test1", "test2" };
    manifest.Protocols = { "htttest" };
    manifest.FileExtensions = { "tst", "test", "testy" };

    index.AddManifest(manifest, "test/id/test.id-1.0.0.yml");
}

TEST_CASE("SQLiteIndexCreateAndAddManifestFile", "[sqliteindex]")
{
    TempFile tempFile{ "repolibtest_tempdb", ".db" };
    INFO("Using temporary file named: " << tempFile.GetPath());

    SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());

    TestDataFile manifestFile{ "GoodManifest.yml" };
    std::filesystem::path manifestPath{ "microsoft/msixsdk/microsoft.msixsdk-1.7.32.yml" };

    index.AddManifest(manifestFile, manifestPath);

    // Attempting to add again should fail
    REQUIRE_THROWS_HR(index.AddManifest(manifestFile, manifestPath), HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));
}

TEST_CASE("PathPartTable_EnsurePathExists_Negative_Paths", "[sqliteindex][V1_0]")
{
    // Open it directly to directly test pathpart table
    Connection connection = Connection::Create(SQLITE_MEMORY_DB_CONNECTION_TARGET, Connection::OpenDisposition::Create);

    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"()", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(\)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(/)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(C:)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(C:\\)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(C:\temp\path\file.txt)", false), E_INVALIDARG);
    REQUIRE_THROWS_HR(Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(\temp\path\file.txt)", false), E_INVALIDARG);
}

TEST_CASE("PathPartTable_EnsurePathExists", "[sqliteindex][V1_0]")
{
    TempFile tempFile{ "repolibtest_tempdb", ".db" };
    INFO("Using temporary file named: " << tempFile.GetPath());

    // Create the index
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, { 1, 0 });
        Schema::Version versionCreated = index.GetVersion();
        REQUIRE(versionCreated == Schema::Version{ 1, 0 });
    }

    // Open it directly to directly test pathpart table
    Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

    // attempt to find path that doesn't exist
    auto result0 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", false);
    REQUIRE(!std::get<0>(result0));

    // add path
    auto result1 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", true);
    REQUIRE(std::get<0>(result1));

    // Second time trying to create should return false and same id
    auto result2 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", true);
    REQUIRE(!std::get<0>(result2));
    REQUIRE(std::get<1>(result1) == std::get<1>(result2));

    // Trying to find but not create should return true because it exists
    auto result3 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\c.txt)", false);
    REQUIRE(std::get<0>(result3));
    REQUIRE(std::get<1>(result1) == std::get<1>(result3));

    // attempt to find a different file
    auto result4 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\d.txt)", false);
    REQUIRE(!std::get<0>(result4));

    // add a different file
    auto result5 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\d.txt)", true);
    REQUIRE(std::get<0>(result5));
    REQUIRE(std::get<1>(result1) != std::get<1>(result5));

    // add the same file but deeper
    auto result6 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\b\d\c.txt)", true);
    REQUIRE(std::get<0>(result6));
    REQUIRE(std::get<1>(result1) != std::get<1>(result6));

    // get the deeper file with extra separators
    auto result7 = Schema::V1_0::PathPartTable::EnsurePathExists(connection, R"(a\\b\d\\c.txt)", true);
    REQUIRE(!std::get<0>(result7));
    REQUIRE(std::get<1>(result6) == std::get<1>(result7));
}
