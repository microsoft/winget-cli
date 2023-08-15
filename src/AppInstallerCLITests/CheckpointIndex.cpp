// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/CheckpointIndex.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;

TEST_CASE("CheckpointIndexCreateLatestAndReopen", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Schema::Version versionCreated;

    // Create the index
    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, Schema::Version::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index
    {
        INFO("Trying with ReadWrite");
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("CheckpointIndex_WriteMetadata", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string_view testCommand = "install"sv;
    std::string_view testClientVersion = "1.20.1234"sv;
    std::string_view testArguments = "install --id Microsoft.PowerToys";

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        index.SetCommandName(testCommand);
        index.SetClientVersion(testClientVersion);
        index.SetCommandArguments(testArguments);
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(testCommand == index.GetCommandName());
        REQUIRE(testClientVersion == index.GetClientVersion());
        REQUIRE(testArguments == index.GetCommandArguments());
    }
}

TEST_CASE("CheckpointIndex_WriteContextData", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string_view testCheckpoint = "testCheckpoint"sv;
    int testContextData = 3;
    std::string_view testName = "testName";
    std::string_view testValue = "testValue";

    std::string_view testName2 = "exampleName";
    std::string_view testValue2 = "exampleValue";

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        index.AddContextData(testCheckpoint, testContextData, testName, testValue);
        index.AddContextData(testCheckpoint, testContextData, testName2, testValue2);
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(testValue == index.GetContextData(testCheckpoint, testContextData, testName));
        REQUIRE(testValue2 == index.GetContextData(testCheckpoint, testContextData, testName2));

        index.RemoveContextData(testCheckpoint, testContextData);
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(index.IsEmpty());
    }
}
