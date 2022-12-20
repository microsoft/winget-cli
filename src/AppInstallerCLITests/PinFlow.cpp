// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/PinCommand.h>
#include <Workflows/PinFlow.h>
#include <Microsoft/PinningIndex.h>
#include <AppInstallerRuntime.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Pinning;

void OverrideForOpenPinningIndex(TestContext& context, const std::filesystem::path& indexPath)
{
    context.Override({ OpenPinningIndex, [=](TestContext& context)
        {
            auto pinningIndex = std::filesystem::exists(indexPath) ?
                PinningIndex::Open(indexPath.u8string(), SQLiteStorageBase::OpenDisposition::ReadWrite) :
                PinningIndex::CreateNew(indexPath.u8string());
            context.Add<Execution::Data::PinningIndex>(std::make_shared<PinningIndex>(std::move(pinningIndex)));
        } });
}

TEST_CASE("PinFlow_Add", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    // auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenPinningIndex(addContext, indexFile.GetPath());
    OverrideForCompositeInstalledSource(addContext);
    addContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);

    PinAddCommand pinAdd({});
    pinAdd.Execute(addContext);
    INFO(pinAddOutput.str());

    SECTION("Pin is saved")
    {
        auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
        auto pins = index.GetAllPins();
        REQUIRE(pins.size() == 1);
        REQUIRE(pins[0].GetType() == PinType::Pinning);
        REQUIRE(pins[0].GetPackageId() == "AppInstallerCliTest.TestExeInstaller");
        REQUIRE(pins[0].GetSourceId() == "*TestSource");
        REQUIRE(pins[0].GetVersionString() == "1.0.0.0");
    }
    SECTION("Remove pin")
    {
        std::ostringstream pinRemoveOutput;
        TestContext removeContext{ pinRemoveOutput, std::cin };
        // auto previousThreadGlobals = context.SetForCurrentThread();
        OverrideForOpenPinningIndex(removeContext, indexFile.GetPath());
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
        // auto previousThreadGlobals = context.SetForCurrentThread();
        OverrideForOpenPinningIndex(resetContext, indexFile.GetPath());

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
        // auto previousThreadGlobals = context.SetForCurrentThread();
        OverrideForOpenPinningIndex(updateContext, indexFile.GetPath());
        OverrideForCompositeInstalledSource(updateContext);
        updateContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);
        updateContext.Args.AddArg(Execution::Args::Type::BlockingPin);

        SECTION("Without --force")
        {
            PinAddCommand pinUpdate({});
            pinUpdate.Execute(updateContext);
            INFO(pinUpdateOutput.str());
            REQUIRE_TERMINATED_WITH(updateContext, APPINSTALLER_CLI_ERROR_PIN_ALREADY_EXISTS);

            auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
            auto pins = index.GetAllPins();
            REQUIRE(pins.size() == 1);
            REQUIRE(pins[0].GetType() == PinType::Pinning);
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
            REQUIRE(pins[0].GetType() == PinType::Blocking);
        }
    }
}

TEST_CASE("PinFlow_Add_NotFound", "[PinFlow][workflow]")
{
    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    // auto previousThreadGlobals = context.SetForCurrentThread();
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

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    // auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenPinningIndex(addContext, indexFile.GetPath());
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
}

TEST_CASE("PinFlow_ListMultiple", "[PinFlow][workflow]")
{
}

TEST_CASE("PinFlow_ListUninstalled", "[PinFlow][workflow]")
{
}

TEST_CASE("PinFlow_RemoveNonExisting", "[PinFlow][workflow]")
{
}

TEST_CASE("PinFlow_ResetEmpty", "[PinFlow][workflow]")
{
}