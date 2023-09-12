// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/CheckpointRecord.h>
#include <Checkpoint.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;
using namespace AppInstaller::Checkpoints;

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

    std::string_view testCheckpointName = "testCheckpoint"sv;
    std::string testCommand = "install";
    std::string testClientVersion = "1.20.1234";

    {
        CheckpointRecord record = CheckpointRecord::CreateNew(tempFile, { 1, 0 });
        CheckpointRecord::IdType checkpointId = record.AddCheckpoint(testCheckpointName);
        record.SetDataValue(checkpointId, AutomaticCheckpointData::CommandName, {}, { testCommand });
        record.SetDataValue(checkpointId, AutomaticCheckpointData::ClientVersion, {}, { testClientVersion });
    }

    {
        CheckpointRecord record = CheckpointRecord::Open(tempFile);
        std::optional<CheckpointRecord::IdType> checkpointId = record.GetCheckpointIdByName(testCheckpointName);
        REQUIRE(checkpointId.has_value());
        REQUIRE(testCommand == record.GetDataSingleValue(checkpointId.value(), AutomaticCheckpointData::CommandName));
        REQUIRE(testClientVersion == record.GetDataSingleValue(checkpointId.value(), AutomaticCheckpointData::ClientVersion));
    }
}

TEST_CASE("CheckpointRecord_WriteContextData", "[checkpointRecord]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string_view testCheckpoint = "testCheckpoint"sv;

    std::string fieldName1 = "field1";
    std::string fieldName2 = "field2";

    std::string testValue1 = "value1";
    std::string testValue2 = "value2";
    std::string testValue3 = "value3";

    {
        CheckpointRecord record = CheckpointRecord::CreateNew(tempFile, { 1, 0 });
        CheckpointRecord::IdType checkpointId = record.AddCheckpoint(testCheckpoint);

        // Add multiple fields.
        record.SetDataValue(checkpointId, AutomaticCheckpointData::Arguments, fieldName1, { testValue1 });
        record.SetDataValue(checkpointId, AutomaticCheckpointData::Arguments, fieldName2, { testValue2, testValue3 });
    }

    {
        CheckpointRecord record = CheckpointRecord::Open(tempFile);
        std::optional<CheckpointRecord::IdType> checkpointId = record.GetCheckpointIdByName(testCheckpoint);
        REQUIRE(checkpointId.has_value());

        const auto& fieldNames = record.GetDataFieldNames(checkpointId.value(), AutomaticCheckpointData::Arguments);

        REQUIRE(fieldNames[0] == fieldName1);
        REQUIRE(fieldNames[1] == fieldName2);

        REQUIRE(testValue1 == record.GetDataFieldSingleValue(checkpointId.value(), AutomaticCheckpointData::Arguments, fieldName1));

        const auto& multiValues = record.GetDataFieldMultiValue(checkpointId.value(), AutomaticCheckpointData::Arguments, fieldName2);

        REQUIRE(testValue2 == multiValues[0]);
        REQUIRE(testValue3 == multiValues[1]);
    }
}

TEST_CASE("CheckpointRecord_CheckpointOrder", "[checkpointRecord]")
{
    // Verifies that the checkpoints are shown in reverse order (latest first).
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string_view firstCheckpoint = "firstCheckpoint"sv;
    std::string_view secondCheckpoint = "secondCheckpoint"sv;
    std::string_view thirdCheckpoint = "thirdCheckpoint"sv;

    {
        CheckpointRecord record = CheckpointRecord::CreateNew(tempFile, { 1, 0 });
        record.AddCheckpoint(firstCheckpoint);
        record.AddCheckpoint(secondCheckpoint);
        record.AddCheckpoint(thirdCheckpoint);
    }

    {
        CheckpointRecord record = CheckpointRecord::Open(tempFile);
        const auto& checkpoints = record.GetCheckpoints();

        REQUIRE(checkpoints[0] == thirdCheckpoint);
        REQUIRE(checkpoints[1] == secondCheckpoint);
        REQUIRE(checkpoints[2] == firstCheckpoint);
    }
}
