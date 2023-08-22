// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/CheckpointRecord.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;

TEST_CASE("CheckpointRecordCreateLatestAndReopen", "[checkpointRecord]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Schema::Version versionCreated;

    // Create the record
    {
        CheckpointRecord record = CheckpointRecord::CreateNew(tempFile, Schema::Version::Latest());
        versionCreated = record.GetVersion();
    }

    // Reopen the record
    {
        INFO("Trying with ReadWrite");
        CheckpointRecord record = CheckpointRecord::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        Schema::Version versionRead = record.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("CheckpointRecord_WriteMetadata", "[checkpointRecord]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string_view testCommand = "install"sv;
    std::string_view testClientVersion = "1.20.1234"sv;
    std::string_view testArguments = "install --id Microsoft.PowerToys";

    {
        CheckpointRecord record = CheckpointRecord::CreateNew(tempFile, { 1, 0 });
        record.SetMetadata(CheckpointMetadata::CommandName, testCommand);
        record.SetMetadata(CheckpointMetadata::ClientVersion, testClientVersion);
    }

    {
        CheckpointRecord record = CheckpointRecord::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(testCommand == record.GetMetadata(CheckpointMetadata::CommandName));
        REQUIRE(testClientVersion == record.GetMetadata(CheckpointMetadata::ClientVersion));
    }
}

TEST_CASE("CheckpointRecord_WriteContextData", "[checkpointRecord]")
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
        CheckpointRecord record = CheckpointRecord::CreateNew(tempFile, { 1, 0 });
        record.AddCheckpoint(testCheckpoint);
        record.AddContextData(testCheckpoint, testContextData, testName, testValue, 0);
        record.AddContextData(testCheckpoint, testContextData, testName2, testValue2, 1);
    }

    {
        CheckpointRecord record = CheckpointRecord::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        auto contextData = record.GetContextData(testCheckpoint, testContextData);
        REQUIRE(contextData.size() == 2);
        REQUIRE(testValue == contextData[0]);
        REQUIRE(testValue2 == contextData[1]);
        record.RemoveContextData(testCheckpoint, testContextData);
    }

    {
        CheckpointRecord record = CheckpointRecord::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(record.IsEmpty());
    }
}
