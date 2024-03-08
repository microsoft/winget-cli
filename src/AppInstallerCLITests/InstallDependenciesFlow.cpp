// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "DependenciesTestSource.h"
#include <Commands/InstallCommand.h>
#include <Workflows/DependenciesFlow.h>
#include <Workflows/InstallFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository;

void OverrideOpenSourceForDependencies(TestContext& context)
{
    context.Override({ "OpenSource", [](TestContext& context)
    {
        context.Add<Execution::Data::Source>(Source{ std::make_shared<DependenciesTestSource>() });
    } });

    context.Override({ Workflow::OpenDependencySource, [](TestContext& context)
    {
        context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    } });
}

void OverrideForProcessMultiplePackages(TestContext& context)
{
    context.Override({ Workflow::ProcessMultiplePackages(
        Resource::String::PackageRequiresDependencies,
        APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES,
        {},
        false,
        true), [](TestContext&)
    {

    } });
}

TEST_CASE("DependencyGraph_SkipInstalled", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    Manifest manifest = CreateFakeManifestWithDependencies("DependenciesInstalled");
    OverrideOpenDependencySource(context);

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    context << CreateDependencySubContexts(Resource::String::PackageRequiresDependencies);

    auto& dependencyPackages = context.Get<Execution::Data::PackageSubContexts>();
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);
    REQUIRE(dependencyPackages.size() == 0);
}

TEST_CASE("DependencyGraph_validMinVersions", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    Manifest manifest = CreateFakeManifestWithDependencies("DependenciesValidMinVersions");
    OverrideOpenDependencySource(context);

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    context << CreateDependencySubContexts(Resource::String::PackageRequiresDependencies);

    auto& dependencyPackages = context.Get<Execution::Data::PackageSubContexts>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);
    REQUIRE(dependencyPackages.size() == 1);
    REQUIRE(dependencyPackages.at(0)->Get<Execution::Data::Manifest>().Id == "minVersion");
    // minVersion 1.5 is available but this requires 1.0 so that version is installed
    REQUIRE(dependencyPackages.at(0)->Get<Execution::Data::Manifest>().Version == "1.0");
}

TEST_CASE("DependencyGraph_PathNoLoop", "[InstallFlow][workflow][dependencyGraph][dependencies]", )
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    Manifest manifest = CreateFakeManifestWithDependencies("PathBetweenBranchesButNoLoop");
    OverrideOpenDependencySource(context);

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    context << CreateDependencySubContexts(Resource::String::PackageRequiresDependencies);

    auto& dependencyPackages = context.Get<Execution::Data::PackageSubContexts>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(dependencyPackages.size() == 4);
    REQUIRE(dependencyPackages.at(0)->Get<Execution::Data::Manifest>().Id == "B");
    REQUIRE(dependencyPackages.at(1)->Get<Execution::Data::Manifest>().Id == "C");
    REQUIRE(dependencyPackages.at(2)->Get<Execution::Data::Manifest>().Id == "G");
    REQUIRE(dependencyPackages.at(3)->Get<Execution::Data::Manifest>().Id == "H");
}

TEST_CASE("DependencyGraph_StackOrderIsOk", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    std::vector<Dependency> installationOrder;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenSourceForDependencies(context);
    OverrideForShellExecute(context, installationOrder);

    context.Args.AddArg(Execution::Args::Type::Query, "StackOrderIsOk"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(installationOrder.size() == 3);
    REQUIRE(installationOrder.at(0).Id() == "B");
    REQUIRE(installationOrder.at(1).Id() == "C");
    REQUIRE(installationOrder.at(2).Id() == "StackOrderIsOk");
}

TEST_CASE("DependencyGraph_MultipleDependenciesFromManifest", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    std::vector<Dependency> installationOrder;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenSourceForDependencies(context);
    OverrideForShellExecute(context, installationOrder);
    OverrideEnableWindowsFeaturesDependencies(context);

    context.Args.AddArg(Execution::Args::Type::Query, "MultipleDependenciesFromManifest"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(installationOrder.size() == 3);
    REQUIRE(installationOrder.at(0).Id() == "Dependency1");
    REQUIRE(installationOrder.at(1).Id() == "Dependency2");
    REQUIRE(installationOrder.at(2).Id() == "AppInstallerCliTest.TestExeInstaller.MultipleDependencies");
}

TEST_CASE("InstallerWithoutDependencies_RootDependenciesAreUsed", "[dependencies]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideOpenDependencySource(context);
    OverrideEnableWindowsFeaturesDependencies(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_DependenciesOnRoot.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify root dependencies are shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageRequiresDependencies).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("PreviewIISOnRoot") != std::string::npos);
}

TEST_CASE("InstallerWithDependencies_SkipDependencies", "[dependencies]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_Dependencies.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::SkipDependencies);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesSkippedMessage).get()) != std::string::npos);
    REQUIRE_FALSE(installOutput.str().find(Resource::LocString(Resource::String::PackageRequiresDependencies).get()) != std::string::npos);
    REQUIRE_FALSE(installOutput.str().find("PreviewIIS") != std::string::npos);
}

TEST_CASE("InstallerWithDependencies_IgnoreDependenciesSetting", "[dependencies]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_Dependencies.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::InstallSkipDependencies>({ true });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesSkippedMessage).get()) != std::string::npos);
    REQUIRE_FALSE(installOutput.str().find(Resource::LocString(Resource::String::PackageRequiresDependencies).get()) != std::string::npos);
    REQUIRE_FALSE(installOutput.str().find("PreviewIIS") != std::string::npos);
}

TEST_CASE("DependenciesMultideclaration_InstallerDependenciesPreference", "[dependencies]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideOpenDependencySource(context);
    OverrideEnableWindowsFeaturesDependencies(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_DependenciesMultideclaration.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify installer dependencies are shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageRequiresDependencies).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("PreviewIIS") != std::string::npos);
    // and root dependencies are not
    REQUIRE(installOutput.str().find("PreviewIISOnRoot") == std::string::npos);
}

TEST_CASE("InstallFlow_Dependencies", "[InstallFlow][workflow][dependencies]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideOpenDependencySource(context);
    OverrideEnableWindowsFeaturesDependencies(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_Dependencies.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageRequiresDependencies).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("PreviewIIS") != std::string::npos);
}

// TODO:
// add dependencies for installer tests to DependenciesTestSource (or a new one)
// add tests for min version dependency solving
// add tests that check for correct installation of dependencies (not only the order)