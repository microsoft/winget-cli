// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include <Commands/PinCommand.h>
#include <Workflows/PinFlow.h>
#include <Microsoft/PinningIndex.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerVersions.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Pinning;

TEST_CASE("PinFlow_Add", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    OverrideForCompositeInstalledSource(addContext);
    addContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);
    addContext.Args.AddArg(Execution::Args::Type::BlockingPin);

    PinAddCommand pinAdd({});
    pinAdd.Execute(addContext);
    INFO(pinAddOutput.str());

    SECTION("Pin is saved")
    {
        auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
        auto pins = index.GetAllPins();
        REQUIRE(pins.size() == 1);
        REQUIRE(pins[0].GetType() == PinType::Blocking);
        REQUIRE(pins[0].GetPackageId() == "AppInstallerCliTest.TestExeInstaller");
        REQUIRE(pins[0].GetSourceId() == "*TestSource");
        REQUIRE(pins[0].GetVersion() == Version{ "1.0.0.0" });

        std::ostringstream pinListOutput;
        TestContext listContext{ pinListOutput, std::cin };
        OverrideForCompositeInstalledSource(listContext);
        listContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);

        PinListCommand pinList({});
        pinList.Execute(listContext);

        INFO(pinListOutput.str());
        REQUIRE(pinListOutput.str().find("AppInstallerCliTest.TestExeInstaller"));
        REQUIRE(pinListOutput.str().find("Blocking"));
    }
    SECTION("Remove pin")
    {
        std::ostringstream pinRemoveOutput;
        TestContext removeContext{ pinRemoveOutput, std::cin };
        OverrideForCompositeInstalledSource(removeContext);
        removeContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);

        PinRemoveCommand pinRemove({});
        pinRemove.Execute(removeContext);
        INFO(pinRemoveOutput.str());

        auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
        auto pins = index.GetAllPins();
        REQUIRE(pins.empty());
    }
    SECTION("Reset pins")
    {
        std::ostringstream pinResetOutput;
        TestContext resetContext{ pinResetOutput, std::cin };
        OverrideForCompositeInstalledSource(resetContext);

        SECTION("Without --force")
        {
            PinResetCommand pinReset({});
            pinReset.Execute(resetContext);
            INFO(pinResetOutput.str());

            auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
            auto pins = index.GetAllPins();
            REQUIRE(pins.size() == 1);
        }
        SECTION("With --force")
        {
            resetContext.Args.AddArg(Execution::Args::Type::Force);

            PinResetCommand pinReset({});
            pinReset.Execute(resetContext);
            INFO(pinResetOutput.str());

            auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
            auto pins = index.GetAllPins();
            REQUIRE(pins.empty());
        }
    }
    SECTION("Update pin")
    {
        std::ostringstream pinUpdateOutput;
        TestContext updateContext{ pinUpdateOutput, std::cin };
        OverrideForCompositeInstalledSource(updateContext);
        updateContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);

        SECTION("Without --force")
        {
            PinAddCommand pinUpdate({});
            pinUpdate.Execute(updateContext);
            INFO(pinUpdateOutput.str());
            REQUIRE_TERMINATED_WITH(updateContext, APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS);

            auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
            auto pins = index.GetAllPins();
            REQUIRE(pins.size() == 1);
            REQUIRE(pins[0].GetType() == PinType::Blocking);
        }
        SECTION("With --force")
        {
            updateContext.Args.AddArg(Execution::Args::Type::Force);

            PinAddCommand pinUpdate({});
            pinUpdate.Execute(updateContext);
            INFO(pinUpdateOutput.str());

            auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
            auto pins = index.GetAllPins();
            REQUIRE(pins.size() == 1);
            REQUIRE(pins[0].GetType() == PinType::Pinning);
        }
    }
}

TEST_CASE("PinFlow_Add_NotFound", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    OverrideForCompositeInstalledSource(addContext);
    addContext.Args.AddArg(Execution::Args::Type::Query, "This package doesn't exist"sv);

    PinAddCommand pinAdd({});
    pinAdd.Execute(addContext);
    INFO(pinAddOutput.str());

    REQUIRE_TERMINATED_WITH(addContext, APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
}

TEST_CASE("PinFlow_Add_NotInstalled", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    OverrideForCompositeInstalledSource(addContext);
    addContext.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithNothingInstalled"sv);

    PinAddCommand pinAdd({});
    pinAdd.Execute(addContext);
    INFO(pinAddOutput.str());

    REQUIRE_TERMINATED_WITH(addContext, APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);

    auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
    auto pins = index.GetAllPins();
    REQUIRE(pins.empty());
}

TEST_CASE("PinFlow_ListEmpty", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinListOutput;
    TestContext listContext{ pinListOutput, std::cin };
    OverrideForCompositeInstalledSource(listContext);

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(pinListOutput.str());

     REQUIRE(pinListOutput.str().find(Resource::LocString(Resource::String::PinNoPinsExist)) != std::string::npos);
}

TEST_CASE("PinFlow_RemoveNonExisting", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinRemoveOutput;
    TestContext removeContext{ pinRemoveOutput, std::cin };
    OverrideForCompositeInstalledSource(removeContext);
    removeContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);

    PinRemoveCommand pinRemove({});
    pinRemove.Execute(removeContext);
    INFO(pinRemoveOutput.str());

    REQUIRE_TERMINATED_WITH(removeContext, APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
}

TEST_CASE("PinFlow_ResetEmpty", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinResetOutput;
    TestContext resetContext{ pinResetOutput, std::cin };
    OverrideForCompositeInstalledSource(resetContext);
    resetContext.Args.AddArg(Execution::Args::Type::Force);

    PinResetCommand pinReset({});
    pinReset.Execute(resetContext);
    INFO(pinResetOutput.str());

    REQUIRE(pinResetOutput.str().find(Resource::LocString(Resource::String::PinNoPinsExist)) != std::string::npos);
}