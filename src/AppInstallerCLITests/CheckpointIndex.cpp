// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/CheckpointIndex.h>
#include <Commands/InstallCommand.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;

constexpr std::string_view s_ArgumentName_Id = "id"sv;
constexpr std::string_view s_ArgumentName_Version = "version"sv;
constexpr std::string_view s_ArgumentName_AcceptPackageAgreements = "accept-package-agreements"sv;
constexpr std::string_view s_ArgumentName_HashOverride = "ignore-security-hash"sv;

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

TEST_CASE("CheckpointIndexAddContext", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int testContextId = 0;
    std::string_view testCommand = "install"sv;
    std::string_view testClientVersion = "1.20.1234"sv;

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        REQUIRE(index.IsEmpty());
        index.AddContext(testContextId);
        index.SetCommandName(testContextId, testCommand);
        index.SetClientVersion(testClientVersion);
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        int firstContextId = index.GetFirstContextId();
        REQUIRE(testContextId == firstContextId);
        REQUIRE(testCommand == index.GetCommandName(firstContextId));
        REQUIRE(testClientVersion == index.GetClientVersion());

        index.RemoveContext(firstContextId);
        REQUIRE(index.IsEmpty());
    }
}

TEST_CASE("CheckpointIndex_ArgumentsTable_AddRemoveArguments", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int testContextId = 0;
    std::string_view testPackageIdentifier = "AppInstallerTest.TestExeInstaller"sv;
    std::string_view testPackageVersion = "1.2.3.4"sv;
    bool testAcceptPackageAgreements = true;
    bool testHashOverride = true;
    
    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        REQUIRE(index.IsEmpty());
        index.AddContext(testContextId);

        // Verify arguments do not exist.
        REQUIRE_FALSE(index.ContainsArgument(testContextId, s_ArgumentName_Id));
        REQUIRE_FALSE(index.ContainsArgument(testContextId, s_ArgumentName_Version));
        REQUIRE_FALSE(index.ContainsArgument(testContextId, s_ArgumentName_AcceptPackageAgreements));
        REQUIRE_FALSE(index.ContainsArgument(testContextId, s_ArgumentName_HashOverride));

        // Add valid context arguments;
        REQUIRE(index.UpdateArgument(testContextId, s_ArgumentName_Id, testPackageIdentifier));
        REQUIRE(index.UpdateArgument(testContextId, s_ArgumentName_Version, testPackageVersion));
        REQUIRE(index.UpdateArgument(testContextId, s_ArgumentName_AcceptPackageAgreements, testAcceptPackageAgreements));
        REQUIRE(index.UpdateArgument(testContextId, s_ArgumentName_HashOverride, testHashOverride));
    }

    {
        // Open index and verify values.
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        int firstContextId = index.GetFirstContextId();

        // Verify arguments exist.
        REQUIRE(index.ContainsArgument(testContextId, s_ArgumentName_Id));
        REQUIRE(index.ContainsArgument(testContextId, s_ArgumentName_Version));
        REQUIRE(index.ContainsArgument(testContextId, s_ArgumentName_AcceptPackageAgreements));
        REQUIRE(index.ContainsArgument(testContextId, s_ArgumentName_HashOverride));

        // Verify context arguments were added.
        REQUIRE(testPackageIdentifier == index.GetStringArgument(testContextId, "id"sv));
        REQUIRE(testPackageVersion == index.GetStringArgument(testContextId, "version"sv));
        REQUIRE(testAcceptPackageAgreements == index.GetBoolArgument(testContextId, "accept-package-agreements"sv));
        REQUIRE(testHashOverride == index.GetBoolArgument(testContextId, "ignore-security-hash"sv));
        
        index.RemoveContext(firstContextId);
        REQUIRE(index.IsEmpty());
    }
}

TEST_CASE("CheckpointIndex_ContextTable_AddRemoveContext", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int testContextId = 0;

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        REQUIRE(index.IsEmpty());
        index.AddContext(testContextId);
        index.SetLastCheckpoint(testContextId, 0);
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(index.GetLastCheckpoint(testContextId) == 0);
        index.SetLastCheckpoint(testContextId, 1);
        REQUIRE(index.GetLastCheckpoint(testContextId) == 1);

        index.RemoveContext(testContextId);
        REQUIRE(index.IsEmpty());
    }
}

TEST_CASE("CheckpointIndex_VerifyContextOrder", "[checkpointIndex]")
{
    // Verify that the context id order is retrieved in order regardless of what sequence they are added.
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        REQUIRE(index.IsEmpty());
        index.AddContext(3);
        index.AddContext(1);
        index.AddContext(0);
        index.AddContext(2);
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        REQUIRE(index.GetFirstContextId() == 0);
        index.RemoveContext(0);
        REQUIRE(index.GetFirstContextId() == 1);
        index.RemoveContext(1);
        REQUIRE(index.GetFirstContextId() == 2);

        // The value should persist if no context was removed.
        REQUIRE(index.GetFirstContextId() == 2);
        index.RemoveContext(2);
        REQUIRE(index.GetFirstContextId() == 3);
        index.RemoveContext(3);

        REQUIRE(index.IsEmpty());
    }
}

std::vector<std::string_view> GetBoolArgsFromCommand(Command command)
{
    const auto& commandArguments = command.GetArguments();
    std::vector<std::string_view> boolArguments;

    for (const auto& argument : commandArguments)
    {
        if (argument.Type() == ArgumentType::Flag)
        {
            boolArguments.emplace_back(argument.Name());
        }
    }

    return boolArguments;
}

std::vector<std::string_view> GetStringArgsFromCommand(Command command)
{
    const auto& commandArguments = command.GetArguments();
    std::vector<std::string_view> stringArguments;

    for (const auto& argument : commandArguments)
    {
        if (argument.Type() != ArgumentType::Flag)
        {
            stringArguments.emplace_back(argument.Name());
        }
    }

    return stringArguments;
}

TEST_CASE("CheckpointIndex_ArgumentTable_VerifyAllArguments_InstallCommand", "[checkpointIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    int testContextId = 0;

    InstallCommand installCommand({});
    std::vector<std::string_view> boolArguments = GetBoolArgsFromCommand(installCommand);
    std::vector<std::string_view> stringArguments = GetStringArgsFromCommand(installCommand);

    {
        CheckpointIndex index = CheckpointIndex::CreateNew(tempFile, { 1, 0 });
        index.AddContext(testContextId);

        for (const auto& argName : boolArguments)
        {
            REQUIRE_FALSE(index.ContainsArgument(testContextId, argName));
            REQUIRE(index.UpdateArgument(testContextId, argName, true));
        }

        std::string_view testValue = "testValue"sv;
        for (const auto& argName : stringArguments)
        {
            REQUIRE_FALSE(index.ContainsArgument(testContextId, argName));
            REQUIRE(index.UpdateArgument(testContextId, argName, testValue));
        }
    }

    {
        CheckpointIndex index = CheckpointIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);

        for (const auto& argName : boolArguments)
        {
            REQUIRE(index.ContainsArgument(testContextId, argName));
            REQUIRE(index.GetBoolArgument(testContextId, argName));
        }

        std::string_view testValue = "testValue"sv;
        for (const auto& argName : stringArguments)
        {
            REQUIRE(index.ContainsArgument(testContextId, argName));
            REQUIRE(testValue == index.GetStringArgument(testContextId, argName));
        }

        index.RemoveContext(testContextId);
        REQUIRE(index.IsEmpty());
    }
}
