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
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::WinGet, PolicyState::Disabled);

    SECTION("Install is blocked")
    {
        std::ostringstream output;
        Execution::Context context{ output, std::cin };
        context.Args.AddArg(Execution::Args::Type::Query, "Fake.Package"sv);
        InstallCommand installCommand({});

        REQUIRE_POLICY_EXCEPTION(
            installCommand.Execute(context),
            TogglePolicy::Policy::WinGet);
    }
    SECTION("Info is not blocked")
    {
        std::ostringstream output;
        Execution::Context context{ output, std::cin };
        context.Args.AddArg(Execution::Args::Type::Info);
        RootCommand rootCommand({});

        rootCommand.Execute(context);

        REQUIRE_FALSE(context.IsTerminated());
    }
}

TEST_CASE("GroupPolicy_SettingsCommand", "[groupPolicy]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::Settings, PolicyState::Disabled);

    Invocation inv{ std::vector<std::string>{ "settings" } };
    RootCommand rootCommand;
    REQUIRE_THROWS(rootCommand.FindSubCommand(inv));
}

TEST_CASE("GroupPolicy_LocalManifests", "[groupPolicy]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);

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

TEST_CASE("GroupPolicy_Info", "[groupPolicy]")
{
    GroupPolicyTestOverride policies;

    std::ostringstream output;
    Execution::Context context{ output, std::cin };
    context.Args.AddArg(Execution::Args::Type::Info);
    RootCommand rootCommand({});

    SECTION("Does not list not configured")
    {
        rootCommand.Execute(context);
        INFO(output.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Group Policy") == std::string::npos);
    }
    SECTION("Shows enabled policies")
    {
        policies.SetState(TogglePolicy::Policy::HashOverride, PolicyState::Enabled);

        rootCommand.Execute(context);
        INFO(output.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Group Policy") != std::string::npos);
        REQUIRE(output.str().find("Hash Override Enabled") != std::string::npos);
    }
    SECTION("Shows disabled policies")
    {
        policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);

        rootCommand.Execute(context);
        INFO(output.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Group Policy") != std::string::npos);
        REQUIRE(output.str().find("Local Manifest Files Disabled") != std::string::npos);
    }
    SECTION("Shows auto update interval")
    {
        policies.SetValue<ValuePolicy::SourceAutoUpdateIntervalInMinutes>(60);

        rootCommand.Execute(context);
        INFO(output.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Group Policy") != std::string::npos);
        REQUIRE(output.str().find("Source Auto Update Interval In Minutes 60") != std::string::npos);
    }
    SECTION("Shows additional sources list")
    {
        SourceFromPolicy source;
        source.Name = "policy-source";
        source.Type = "Test.Type";
        source.Arg = "test-arg";
        policies.SetState(TogglePolicy::Policy::AdditionalSources, PolicyState::Enabled);
        policies.SetValue<ValuePolicy::AdditionalSources>({ source });

        rootCommand.Execute(context);
        INFO(output.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Group Policy") != std::string::npos);
        REQUIRE(output.str().find("Sources Enabled") != std::string::npos);
        REQUIRE(output.str().find("Additional source") != std::string::npos);
        REQUIRE(output.str().find(source.Name) != std::string::npos);
        REQUIRE(output.str().find(source.Type) != std::string::npos);
        REQUIRE(output.str().find(source.Arg) != std::string::npos);
    }
    SECTION("Shows allowed sources list")
    {
        SourceFromPolicy source;
        source.Name = "allowed-source";
        source.Type = "Test.Type";
        source.Arg = "test-arg";
        policies.SetState(TogglePolicy::Policy::AllowedSources, PolicyState::Enabled);
        policies.SetValue<ValuePolicy::AllowedSources>({ source });

        rootCommand.Execute(context);
        INFO(output.str());

        REQUIRE_FALSE(context.IsTerminated());
        REQUIRE(output.str().find("Group Policy") != std::string::npos);
        REQUIRE(output.str().find("Allowed Sources Enabled") != std::string::npos);
        REQUIRE(output.str().find("Allowed source") != std::string::npos);
        REQUIRE(output.str().find(source.Name) != std::string::npos);
        REQUIRE(output.str().find(source.Type) != std::string::npos);
        REQUIRE(output.str().find(source.Arg) != std::string::npos);
    }
}