// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include "PinTestCommon.h"
#include <AppInstallerDateTime.h>
#include <Commands/PinCommand.h>
#include <Workflows/PinFlow.h>
#include <Microsoft/PinningIndex.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerVersions.h>
#include <winget/PinningData.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Pinning;
using namespace AppInstaller::SQLite;

TEST_CASE("PinFlow_Add", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    OverrideForCompositeInstalledSource(addContext, CreateTestSource({ TSR::TestInstaller_Exe }));
    addContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);
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
        REQUIRE(pins[0].GetGatedVersion().ToString() == "");
        REQUIRE(pins[0].GetKey().PackageId == "AppInstallerCliTest.TestExeInstaller");
        REQUIRE(pins[0].GetKey().SourceId == "*TestSource");
        REQUIRE(pins[0].GetDateAdded().has_value());
        REQUIRE_FALSE(pins[0].GetNote().has_value());

        std::ostringstream pinListOutput;
        TestContext listContext{ pinListOutput, std::cin };
        OverrideForCompositeInstalledSource(listContext, CreateTestSource({ TSR::TestInstaller_Exe }));
        listContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);

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
        OverrideForCompositeInstalledSource(removeContext, CreateTestSource({ TSR::TestInstaller_Exe }));
        removeContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);

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

        SECTION("Without --force")
        {
            OverrideForCompositeInstalledSource(resetContext, CreateTestSource({ TSR::TestInstaller_Exe }));
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
        OverrideForCompositeInstalledSource(updateContext, CreateTestSource({ TSR::TestInstaller_Exe }));
        updateContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);

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
    OverrideForCompositeInstalledSource(addContext, CreateTestSource({}));
    addContext.Args.AddArg(Execution::Args::Type::Query, "This package doesn't exist"sv);

    PinAddCommand pinAdd({});
    pinAdd.Execute(addContext);
    INFO(pinAddOutput.str());

    REQUIRE_TERMINATED_WITH(addContext, APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
}

TEST_CASE("PinFlow_ListEmpty", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinListOutput;
    TestContext listContext{ pinListOutput, std::cin };
    OverrideForCompositeInstalledSource(listContext, CreateTestSource({}));

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
    OverrideForCompositeInstalledSource(removeContext, CreateTestSource({ TSR::TestInstaller_Exe }));
    removeContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);

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
    resetContext.Args.AddArg(Execution::Args::Type::Force);

    PinResetCommand pinReset({});
    pinReset.Execute(resetContext);
    INFO(pinResetOutput.str());

    REQUIRE(pinResetOutput.str().find(Resource::LocString(Resource::String::PinNoPinsExist)) != std::string::npos);
}

TEST_CASE("PinFlow_Add_WithNote", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    std::ostringstream pinAddOutput;
    TestContext addContext{ pinAddOutput, std::cin };
    OverrideForCompositeInstalledSource(addContext, CreateTestSource({ TSR::TestInstaller_Exe }));
    addContext.Args.AddArg(Execution::Args::Type::Query, TSR::TestInstaller_Exe.Query);
    addContext.Args.AddArg(Execution::Args::Type::PinNote, "my test note"sv);

    PinAddCommand pinAdd({});
    pinAdd.Execute(addContext);
    INFO(pinAddOutput.str());

    auto index = PinningIndex::Open(indexFile.GetPath().u8string(), SQLiteStorageBase::OpenDisposition::Read);
    auto pins = index.GetAllPins();
    REQUIRE(pins.size() == 1);
    REQUIRE(pins[0].GetNote().has_value());
    REQUIRE(pins[0].GetNote().value() == "my test note");
}

// Helper: Creates a v1.1 pinning index at the given path and adds the provided pins directly.
// Each pin should already have date_added and note set as desired.
namespace
{
    void PopulatePinIndexForShow(const std::filesystem::path& indexPath, const std::vector<Pin>& pins)
    {
        PinningIndex index = PinningIndex::CreateNew(indexPath.u8string(), AppInstaller::SQLite::Version::Latest());
        for (const auto& pin : pins)
        {
            index.AddPin(pin);
        }
    }
}

TEST_CASE("PinFlow_Show_NoMatch", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin existingPin = Pin::CreateBlockingPin({ "SomePackage.Id", "sourceId" });
    existingPin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jan2026_15_1000));
    PopulatePinIndexForShow(indexFile.GetPath(), { existingPin });

    std::ostringstream showOutput;
    TestContext showContext{ showOutput, std::cin };
    showContext.Args.AddArg(Execution::Args::Type::Query, "ThisQueryMatchesNothing"sv);

    PinShowCommand pinShow({});
    pinShow.Execute(showContext);
    INFO(showOutput.str());

    REQUIRE_TERMINATED_WITH(showContext, APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
    REQUIRE(showOutput.str().find(Resource::LocString(Resource::String::PinShowNoMatchFound)) != std::string::npos);
}

TEST_CASE("PinFlow_Show_MatchById", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreateBlockingPin({ "MyApp.Package", "sourceId" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jun2026_01_0900));
    pin.SetNote(std::string{ "keep this one" });
    PopulatePinIndexForShow(indexFile.GetPath(), { pin });

    std::ostringstream showOutput;
    TestContext showContext{ showOutput, std::cin };
    showContext.Args.AddArg(Execution::Args::Type::Id, "MyApp.Package"sv);

    PinShowCommand pinShow({});
    pinShow.Execute(showContext);
    INFO(showOutput.str());

    REQUIRE_FALSE(showContext.IsTerminated());
    REQUIRE(showOutput.str().find("MyApp.Package") != std::string::npos);
    REQUIRE(showOutput.str().find("Blocking") != std::string::npos);
    REQUIRE(showOutput.str().find("Date added:") != std::string::npos);
    REQUIRE(showOutput.str().find("keep this one") != std::string::npos);
}

TEST_CASE("PinFlow_Show_MatchByQuery", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreatePinningPin({ "Contoso.AppOne", "sourceId" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Mar2026_10_1200));
    PopulatePinIndexForShow(indexFile.GetPath(), { pin });

    std::ostringstream showOutput;
    TestContext showContext{ showOutput, std::cin };
    // Partial, case-insensitive match on the package ID
    showContext.Args.AddArg(Execution::Args::Type::Query, "appone"sv);

    PinShowCommand pinShow({});
    pinShow.Execute(showContext);
    INFO(showOutput.str());

    REQUIRE_FALSE(showContext.IsTerminated());
    REQUIRE(showOutput.str().find("Contoso.AppOne") != std::string::npos);
}

TEST_CASE("PinFlow_Show_ExactMatch", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    // Two pins sharing a prefix
    Pin pinA = Pin::CreateBlockingPin({ "Vendor.App", "src" });
    pinA.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jan2026_01_0000));

    Pin pinB = Pin::CreateBlockingPin({ "Vendor.AppExtra", "src" });
    pinB.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jan2026_01_0000));

    PopulatePinIndexForShow(indexFile.GetPath(), { pinA, pinB });

    std::ostringstream showOutput;
    TestContext showContext{ showOutput, std::cin };
    showContext.Args.AddArg(Execution::Args::Type::Id, "Vendor.App"sv);
    showContext.Args.AddArg(Execution::Args::Type::Exact);

    PinShowCommand pinShow({});
    pinShow.Execute(showContext);
    INFO(showOutput.str());

    REQUIRE_FALSE(showContext.IsTerminated());
    // Only the exact-match pin should appear
    REQUIRE(showOutput.str().find("Vendor.App") != std::string::npos);
    // The inexact match should NOT appear
    REQUIRE(showOutput.str().find("Vendor.AppExtra") == std::string::npos);
}

TEST_CASE("PinFlow_Show_NoNote_DoesNotShowNoteLabel", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreatePinningPin({ "NoNote.Package", "src" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::May2026_01_0800));
    // note intentionally not set
    PopulatePinIndexForShow(indexFile.GetPath(), { pin });

    std::ostringstream showOutput;
    TestContext showContext{ showOutput, std::cin };
    showContext.Args.AddArg(Execution::Args::Type::Query, "NoNote.Package"sv);

    PinShowCommand pinShow({});
    pinShow.Execute(showContext);
    INFO(showOutput.str());

    REQUIRE_FALSE(showContext.IsTerminated());
    REQUIRE(showOutput.str().find("NoNote.Package") != std::string::npos);
    REQUIRE(showOutput.str().find(Resource::LocString(Resource::String::PinShowLabelNote)) == std::string::npos);
}

TEST_CASE("PinFlow_Show_EmptyIndex_NoMatch", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    // Create an empty index (no pins)
    { PinningIndex::CreateNew(indexFile.GetPath().u8string(), AppInstaller::SQLite::Version::Latest()); }

    std::ostringstream showOutput;
    TestContext showContext{ showOutput, std::cin };
    showContext.Args.AddArg(Execution::Args::Type::Query, "AnyQuery"sv);

    PinShowCommand pinShow({});
    pinShow.Execute(showContext);
    INFO(showOutput.str());

    REQUIRE_TERMINATED_WITH(showContext, APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
}
