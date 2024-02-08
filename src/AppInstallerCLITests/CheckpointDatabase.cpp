// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/CheckpointDatabase.h>
#include <Public/winget/Checkpoint.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;
using namespace AppInstaller::Checkpoints;

TEST_CASE("CheckpointDatabaseCreateLatestAndReopen", "[checkpointDatabase]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Version versionCreated;

    // Create the database
    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::CreateNew(tempFile, Version::Latest());
        versionCreated = database->GetVersion();
    }

    // Reopen the database
    {
        INFO("Trying with ReadWrite");
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        Version versionRead = database->GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("CheckpointDatabase_WriteAndRemoveMetadata", "[checkpointDatabase]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    std::string_view testCheckpointName = "testCheckpoint"sv;
    std::string testCommand = "install";
    std::string testClientVersion = "1.20.1234";

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::CreateNew(tempFile, { 1, 0 });
        CheckpointDatabase::IdType checkpointId = database->AddCheckpoint(testCheckpointName);
        database->SetDataValue(checkpointId, AutomaticCheckpointData::Command, {}, { testCommand });
        database->SetDataValue(checkpointId, AutomaticCheckpointData::ClientVersion, {}, { testClientVersion });
    }

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::Open(tempFile);
        const auto& checkpointIds = database->GetCheckpointIds();
        REQUIRE_FALSE(checkpointIds.empty());

        auto checkpointId = checkpointIds[0];
        REQUIRE(testCommand == database->GetDataFieldSingleValue(checkpointId, AutomaticCheckpointData::Command, {}));
        REQUIRE(testClientVersion == database->GetDataFieldSingleValue(checkpointId, AutomaticCheckpointData::ClientVersion, {}));

        database->RemoveDataType(checkpointId, AutomaticCheckpointData::Command);
        database->RemoveDataType(checkpointId, AutomaticCheckpointData::ClientVersion);
    }

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::Open(tempFile);
        const auto& checkpointIds = database->GetCheckpointIds();
        REQUIRE_FALSE(checkpointIds.empty());

        auto checkpointId = checkpointIds[0];
        REQUIRE(database->GetDataTypes(checkpointId).empty());
    }
}

TEST_CASE("CheckpointDatabase_WriteContextData", "[checkpointDatabase]")
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
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::CreateNew(tempFile, { 1, 0 });
        CheckpointDatabase::IdType checkpointId = database->AddCheckpoint(testCheckpoint);

        // Add multiple fields.
        database->SetDataValue(checkpointId, AutomaticCheckpointData::Arguments, fieldName1, { testValue1 });
        database->SetDataValue(checkpointId, AutomaticCheckpointData::Arguments, fieldName2, { testValue2, testValue3 });
    }

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::Open(tempFile);
        const auto& checkpointIds = database->GetCheckpointIds();
        REQUIRE_FALSE(checkpointIds.empty());

        auto automaticCheckpointId = checkpointIds.back();

        const auto& fieldNames = database->GetDataFieldNames(automaticCheckpointId, AutomaticCheckpointData::Arguments);

        REQUIRE(fieldNames[0] == fieldName1);
        REQUIRE(fieldNames[1] == fieldName2);

        REQUIRE(testValue1 == database->GetDataFieldSingleValue(automaticCheckpointId, AutomaticCheckpointData::Arguments, fieldName1));

        const auto& multiValues = database->GetDataFieldMultiValue(automaticCheckpointId, AutomaticCheckpointData::Arguments, fieldName2);

        REQUIRE(testValue2 == multiValues[0]);
        REQUIRE(testValue3 == multiValues[1]);
    }
}

TEST_CASE("CheckpointDatabase_CheckpointOrder", "[checkpointDatabase]")
{
    // Verifies that the checkpoints are shown in reverse order (latest first).
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::CreateNew(tempFile, { 1, 0 });
        database->AddCheckpoint("firstCheckpoint"sv);
        database->AddCheckpoint("secondCheckpoint"sv);
        database->AddCheckpoint("thirdCheckpoint"sv);
    }

    {
        std::shared_ptr<CheckpointDatabase> database = CheckpointDatabase::Open(tempFile);
        const auto& checkpointIds = database->GetCheckpointIds();
        REQUIRE(checkpointIds.size() == 3);
        REQUIRE(checkpointIds[0] == 3);
        REQUIRE(checkpointIds[1] == 2);
        REQUIRE(checkpointIds[2] == 1);
    }
}
