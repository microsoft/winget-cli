// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include <Commands/ExportCommand.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;

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
