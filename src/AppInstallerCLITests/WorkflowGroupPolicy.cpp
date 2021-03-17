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

namespace
{
    const std::wstring SourceConfigurationPolicyValueName = L"DisableSourceConfiguration";
    const std::wstring DefaultSourcesPolicyValueName = L"ExcludeDefaultSources";
}

TEST_CASE("GroupPolicy_WinGet", "[groupPolicy]")
{
    auto reset = PrepareGroupPolicyForTest();
    SetGroupPolicy(L"DisableWinGet", true);

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
        // TODO: Find policy in output
        REQUIRE(output.str().find("Disable") != std::string::npos);
    }
}

TEST_CASE("GroupPolicy_SettingsCommand", "[groupPolicy]")
{
    auto reset = PrepareGroupPolicyForTest();
    SetGroupPolicy(L"DisableSettingsCommand", true);

    Invocation inv{ std::vector<std::string>{ "settings" } };
    RootCommand rootCommand;
    REQUIRE_THROWS(rootCommand.FindSubCommand(inv));
}

TEST_CASE("GroupPolicy_LocalManifests", "[groupPolicy]")
{
    auto reset = PrepareGroupPolicyForTest();
    SetGroupPolicy(L"DisableLocalManifestFiles", true);

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

TEST_CASE("GroupPolicy_SourceConfiguration", "[groupPolicy]")
{
    auto reset = PrepareGroupPolicyForTest();
    SetGroupPolicy(L"DisableSourceConfiguration", true);

    SECTION("add is blocked")
    {
        Invocation inv{ std::vector<std::string>{ "source", "add" } };
        RootCommand rootCommand;
        REQUIRE_NOTHROW(rootCommand.FindSubCommand(inv));
    }
    SECTION("remove is blocked")
    {
        Invocation inv{ std::vector<std::string>{ "source", "remove" } };
        RootCommand rootCommand;
        REQUIRE_NOTHROW(rootCommand.FindSubCommand(inv));
    }
    SECTION("reset is blocked")
    {
        Invocation inv{ std::vector<std::string>{ "source", "reset" } };
        RootCommand rootCommand;
        REQUIRE_NOTHROW(rootCommand.FindSubCommand(inv));
    }
    SECTION("list is allowed")
    {
        Invocation inv{ std::vector<std::string>{ "source", "list" } };
        RootCommand rootCommand;
        REQUIRE_NOTHROW(rootCommand.FindSubCommand(inv));
    }
}

/*
const std::wstring SourceConfigurationPolicyValueName = L"DisableSourceConfiguration";
const std::wstring DefaultSourcesPolicyValueName = L"ExcludeDefaultSources";
*/