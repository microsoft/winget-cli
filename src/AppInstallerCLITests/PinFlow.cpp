// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include "PinTestCommon.h"
#include "TestSource.h"
#include <AppInstallerDateTime.h>
#include <winget/Manifest.h>
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

// Helper: Creates a pinning index at the given path and adds the provided pins directly.
// Each pin should already have date_added and note set as desired.
namespace
{
    AppInstaller::Manifest::Manifest MakeListTestManifest(std::string_view id)
    {
        AppInstaller::Manifest::Manifest result;
        result.Id = std::string{ id };
        result.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>(std::string{ id });
        result.DefaultLocalization.Add<AppInstaller::Manifest::Localization::Publisher>("TestPublisher");
        result.Version = "1.0.0";
        result.Installers.push_back({});
        return result;
    }

    TestSourceResult MakeListTestSourceResult(std::string_view id)
    {
        std::string packageId{ id };
        return TestSourceResult(
            packageId,
            [packageId](std::vector<AppInstaller::Repository::ResultMatch>& matches, std::weak_ptr<const AppInstaller::Repository::ISource> source)
            {
                auto manifest = MakeListTestManifest(packageId);
                matches.emplace_back(
                    AppInstaller::Repository::ResultMatch(
                        TestCompositePackage::Make(std::vector<AppInstaller::Manifest::Manifest>{ manifest }, source),
                        AppInstaller::Repository::PackageMatchFilter(
                            AppInstaller::Repository::PackageMatchField::Id,
                            AppInstaller::Repository::MatchType::Exact,
                            packageId)));
            });
    }

    std::shared_ptr<WorkflowTestSource> CreateListTestSource(std::initializer_list<std::string_view> ids)
    {
        std::vector<TestSourceResult> results;
        results.reserve(ids.size());
        for (auto id : ids)
        {
            results.emplace_back(MakeListTestSourceResult(id));
        }

        return CreateTestSource(std::move(results));
    }

    void PopulatePinIndexForList(const std::filesystem::path& indexPath, const std::vector<Pin>& pins, AppInstaller::SQLite::Version version = AppInstaller::SQLite::Version::Latest())
    {
        PinningIndex index = PinningIndex::CreateNew(indexPath.u8string(), version);
        for (const auto& pin : pins)
        {
            index.AddPin(pin);
        }
    }
}

TEST_CASE("PinFlow_List_Filter_NoMatch", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin existingPin = Pin::CreateBlockingPin({ "SomePackage.Id", "sourceId" });
    existingPin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jan2026_15_1000));
    PopulatePinIndexForList(indexFile.GetPath(), { existingPin });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Query, "ThisQueryMatchesNothing"sv);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "SomePackage.Id" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_TERMINATED_WITH(listContext, APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinShowNoMatchFound)) != std::string::npos);
}

TEST_CASE("PinFlow_List_Filter_MatchById_WithDetails", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreateBlockingPin({ "MyApp.Package", "sourceId" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jun2026_01_0900));
    pin.SetNote(std::string{ "keep this one" });
    PopulatePinIndexForList(indexFile.GetPath(), { pin });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Id, "MyApp.Package"sv);
    listContext.Args.AddArg(Execution::Args::Type::ListDetails);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "MyApp.Package" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_FALSE(listContext.IsTerminated());
    REQUIRE(listOutput.str().find("MyApp.Package") != std::string::npos);
    REQUIRE(listOutput.str().find("Blocking") != std::string::npos);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinDateAdded)) != std::string::npos);
    REQUIRE(listOutput.str().find("keep this one") != std::string::npos);
}

TEST_CASE("PinFlow_List_Filter_MatchByQuery", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreatePinningPin({ "Contoso.AppOne", "sourceId" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Mar2026_10_1200));
    PopulatePinIndexForList(indexFile.GetPath(), { pin });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    // Partial, case-insensitive match on the package ID
    listContext.Args.AddArg(Execution::Args::Type::Query, "appone"sv);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "Contoso.AppOne" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_FALSE(listContext.IsTerminated());
    REQUIRE(listOutput.str().find("Contoso.AppOne") != std::string::npos);
}

TEST_CASE("PinFlow_List_Filter_ExactMatch", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    // Two pins sharing a prefix
    Pin pinA = Pin::CreateBlockingPin({ "Vendor.App", "src" });
    pinA.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jan2026_01_0000));

    Pin pinB = Pin::CreateBlockingPin({ "Vendor.AppExtra", "src" });
    pinB.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jan2026_01_0000));

    PopulatePinIndexForList(indexFile.GetPath(), { pinA, pinB });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Id, "Vendor.App"sv);
    listContext.Args.AddArg(Execution::Args::Type::Exact);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "Vendor.App", "Vendor.AppExtra" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_FALSE(listContext.IsTerminated());
    // Only the exact-match pin should appear
    REQUIRE(listOutput.str().find("Vendor.App") != std::string::npos);
    // The inexact match should NOT appear
    REQUIRE(listOutput.str().find("Vendor.AppExtra") == std::string::npos);
}

TEST_CASE("PinFlow_List_DetailsWithoutNotes_ShowsEmptyNoteColumn", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreatePinningPin({ "NoNote.Package", "src" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::May2026_01_0800));
    // note intentionally not set
    PopulatePinIndexForList(indexFile.GetPath(), { pin });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Query, "NoNote.Package"sv);
    listContext.Args.AddArg(Execution::Args::Type::ListDetails);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "NoNote.Package" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_FALSE(listContext.IsTerminated());
    REQUIRE(listOutput.str().find("NoNote.Package") != std::string::npos);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinDateAdded)) != std::string::npos);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinNote)) != std::string::npos);
}

TEST_CASE("PinFlow_List_V1_0Index_NoMigrationRequired", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreateBlockingPin({ "Legacy.Package", "src" });
    PopulatePinIndexForList(indexFile.GetPath(), { pin }, { 1, 0 });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Query, "Legacy.Package"sv);
    listContext.Args.AddArg(Execution::Args::Type::ListDetails);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "Legacy.Package" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_FALSE(listContext.IsTerminated());
    REQUIRE(listOutput.str().find("Legacy.Package") != std::string::npos);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinDateAdded)) == std::string::npos);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinNote)) == std::string::npos);
}

TEST_CASE("PinFlow_List_Filter_EmptyIndex_NoMatch", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    // Create an empty index (no pins)
    { PinningIndex::CreateNew(indexFile.GetPath().u8string(), AppInstaller::SQLite::Version::Latest()); }

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Query, "AnyQuery"sv);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({}));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_TERMINATED_WITH(listContext, APPINSTALLER_CLI_ERROR_PIN_DOES_NOT_EXIST);
}

TEST_CASE("PinFlow_List_DefaultOutput_DoesNotShowDetailsColumns", "[PinFlow][workflow]")
{
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    Pin pin = Pin::CreateBlockingPin({ "Has.Note.Package", "sourceId" });
    pin.SetDateAdded(AppInstaller::Utility::ConvertUnixEpochToSystemClock(PinTestEpoch::Jun2026_01_0900));
    pin.SetNote(std::string{ "some note" });
    PopulatePinIndexForList(indexFile.GetPath(), { pin });

    std::ostringstream listOutput;
    TestContext listContext{ listOutput, std::cin };
    listContext.Args.AddArg(Execution::Args::Type::Query, "Has.Note.Package"sv);
    OverrideForCompositeInstalledSource(listContext, CreateListTestSource({ "Has.Note.Package" }));

    PinListCommand pinList({});
    pinList.Execute(listContext);
    INFO(listOutput.str());

    REQUIRE_FALSE(listContext.IsTerminated());
    REQUIRE(listOutput.str().find("Has.Note.Package") != std::string::npos);
    REQUIRE(listOutput.str().find(Resource::LocString(Resource::String::PinDateAdded)) == std::string::npos);
}
