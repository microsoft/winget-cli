// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include "AppInstallerErrors.h"
#include "Commands/InstallCommand.h"
#include "Commands/RootCommand.h"
#include "Commands/ShowCommand.h"
#include "Commands/UpgradeCommand.h"
#include "Commands/ValidateCommand.h"

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Settings;
using namespace std::string_view_literals;


TEST_CASE("GroupPolicy_WinGet", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    SetRegistryValue(policiesKey.get(), WinGetPolicyValueName, false);
    GroupPolicyTestOverride policies{ policiesKey.get() };

    SECTION("Install is blocked")
    {
        std::ostringstream output;
        Execution::Context context{ output, std::cin };
        context.Args.AddArg(Execution::Args::Type::Query, "Fake.Package"sv);
        InstallCommand installCommand({});

        installCommand.Execute(context);

        REQUIRE(context.IsTerminated());
        REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY);
    }
    SECTION("Info is not blocked")
    {
        std::ostringstream output;
        Execution::Context context{ output, std::cin };
        context.Args.AddArg(Execution::Args::Type::Info);
        RootCommand rootCommand({});

        rootCommand.Execute(context);

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Enable Windows Package Manager") != std::string::npos);
    }
}

TEST_CASE("GroupPolicy_SettingsCommand", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    SetRegistryValue(policiesKey.get(), WinGetSettingsPolicyValueName, false);
    GroupPolicyTestOverride policies{ policiesKey.get() };

    Invocation inv{ std::vector<std::string>{ "settings" } };
    RootCommand rootCommand;
    REQUIRE_THROWS(rootCommand.FindSubCommand(inv));
}

TEST_CASE("GroupPolicy_LocalManifests", "[groupPolicy]")
{
    auto policiesKey = RegCreateVolatileTestRoot();
    SetRegistryValue(policiesKey.get(), LocalManifestsPolicyValueName, false);
    GroupPolicyTestOverride policies{ policiesKey.get() };

    SECTION("Blocked on install")
    {
        Execution::Args args;
        args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand({});
        REQUIRE_THROWS(installCommand.ValidateArguments(args));
    }
    SECTION("Blocked on upgrade")
    {
        Execution::Args args;
        args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        UpgradeCommand upgradeCommand({});
        REQUIRE_THROWS(upgradeCommand.ValidateArguments(args));
    }
    SECTION("Allowed on show")
    {
        Execution::Args args;
        args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        ShowCommand showCommand({});
        REQUIRE_NOTHROW(showCommand.ValidateArguments(args));
    }
    SECTION("Allowed on validate")
    {
        Execution::Args args;
        args.AddArg(Execution::Args::Type::ValidateManifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        ValidateCommand validateCommand({});
        REQUIRE_NOTHROW(validateCommand.ValidateArguments(args));
    }
}
