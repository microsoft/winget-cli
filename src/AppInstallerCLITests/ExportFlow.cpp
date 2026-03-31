// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestSource.h"
#include <Commands/ExportCommand.h>
#include <winget/ManifestYamlParser.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Manifest::YamlParser;

TEST_CASE("ExportFlow_ExportAll", "[ExportFlow][workflow]")
{
    TestCommon::TempFile exportResultPath("TestExport.json");

    std::ostringstream exportOutput;
    TestContext context{ exportOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    context.Args.AddArg(Execution::Args::Type::OutputFile, exportResultPath);

    ExportCommand exportCommand({});
    exportCommand.Execute(context);
    INFO(exportOutput.str());

    // Verify contents of exported collection
    const auto& exportedCollection = context.Get<Execution::Data::PackageCollection>();
    REQUIRE(exportedCollection.Sources.size() == 1);
    REQUIRE(exportedCollection.Sources[0].Details.Identifier == "*TestSource");

    const auto& exportedPackages = exportedCollection.Sources[0].Packages;
    REQUIRE(exportedPackages.size() == 6);
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestExeInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMsixInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMSStoreInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestPortableInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestZipInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestExeUnknownVersion" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
}

TEST_CASE("ExportFlow_ExportAll_WithVersions", "[ExportFlow][workflow]")
{
    TestCommon::TempFile exportResultPath("TestExport.json");

    std::ostringstream exportOutput;
    TestContext context{ exportOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, CreateTestSource({
        TSR::TestInstaller_Exe,
        TSR::TestInstaller_Exe_UnknownVersion,
        TSR::TestInstaller_Msix,
        TSR::TestInstaller_MSStore,
        TSR::TestInstaller_Portable,
        TSR::TestInstaller_Zip,
        }));
    context.Args.AddArg(Execution::Args::Type::OutputFile, exportResultPath);
    context.Args.AddArg(Execution::Args::Type::IncludeVersions);

    ExportCommand exportCommand({});
    exportCommand.Execute(context);
    INFO(exportOutput.str());

    // Verify contents of exported collection
    const auto& exportedCollection = context.Get<Execution::Data::PackageCollection>();
    REQUIRE(exportedCollection.Sources.size() == 1);
    REQUIRE(exportedCollection.Sources[0].Details.Identifier == "*TestSource");

    const auto& exportedPackages = exportedCollection.Sources[0].Packages;
    REQUIRE(exportedPackages.size() == 6);
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestExeInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMsixInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMSStoreInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestPortableInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestZipInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestExeUnknownVersion" && p.VersionAndChannel.GetVersion().ToString() == "unknown";
        }));
}

TEST_CASE("ExportFlow_ExportAll_WithUserInstallerArgs", "[ExportFlow][workflow]")
{
    TestCommon::TempFile exportResultPath("TestExport.json");

    std::ostringstream exportOutput;
    TestContext context{ exportOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    // Create a test source with packages that have InitialOverrideArguments and InitialCustomSwitches set
    auto testSource = CreateTestSource({});

    TestSourceResult exeWithOverride(
        "AppInstallerCliTest.TestExeInstaller"sv,
        [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source)
        {
            auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
            auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
            auto manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2.yaml"));

            auto testPackage = TestCompositePackage::Make(
                manifest,
                TestCompositePackage::MetadataMap
                {
                    { PackageVersionMetadata::InstalledType, "Exe" },
                    { PackageVersionMetadata::InitialOverrideArguments, "/silent /override" },
                    { PackageVersionMetadata::InitialCustomSwitches, "--custom-flag" },
                },
                std::vector<Manifest>{ manifest3, manifest2, manifest },
                source);
            for (auto& availablePackage : testPackage->Available)
            {
                availablePackage->IsSameOverride = [](const IPackage*, const IPackage*) { return true; };
            }
            matches.emplace_back(
                ResultMatch(
                    testPackage,
                    PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
        });

    testSource->AddResult(exeWithOverride);

    OverrideForCompositeInstalledSource(context, testSource);
    context.Args.AddArg(Execution::Args::Type::OutputFile, exportResultPath);

    ExportCommand exportCommand({});
    exportCommand.Execute(context);
    INFO(exportOutput.str());

    const auto& exportedCollection = context.Get<Execution::Data::PackageCollection>();
    REQUIRE(exportedCollection.Sources.size() == 1);

    const auto& exportedPackages = exportedCollection.Sources[0].Packages;
    REQUIRE(exportedPackages.size() == 1);

    const auto& pkg = exportedPackages[0];
    REQUIRE(pkg.Id == "AppInstallerCliTest.TestExeInstaller");
    REQUIRE(pkg.InitialOverrideArgs == "/silent /override");
    REQUIRE(pkg.InitialCustomSwitches == "--custom-flag");

    // Verify the values are in the exported JSON file
    std::ifstream exportFile(exportResultPath.GetPath());
    Json::Value exportedJson;
    exportFile >> exportedJson;

    const auto& jsonPackage = exportedJson["Sources"][0]["Packages"][0];
    REQUIRE(jsonPackage["InitialOverrideArguments"].asString() == "/silent /override");
    REQUIRE(jsonPackage["InitialCustomSwitches"].asString() == "--custom-flag");
}
