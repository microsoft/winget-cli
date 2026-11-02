// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestSettings.h"
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <winget/Settings.h>
#include <Workflows/DownloadFlow.h>
#include <Commands/DscUserSettingsFileResource.h>
#include <Commands/InstallCommand.h>
#include <Commands/SettingsCommand.h>
#include <Commands/ValidateCommand.h>
#include <Workflows/SettingsFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

void VerifyMotw(const std::filesystem::path& testFile, DWORD zone)
{
    std::filesystem::path motwFile(testFile);
    motwFile += ":Zone.Identifier:$data";
    std::ifstream motwStream(motwFile);
    std::stringstream motwContent;
    motwContent << motwStream.rdbuf();
    std::string motwContentStr = motwContent.str();
    motwStream.close();
    REQUIRE(motwContentStr.find("ZoneId=" + std::to_string(zone)) != std::string::npos);
}

TEST_CASE("VerifyInstallerTrustLevelAndUpdateInstallerFileMotw", "[DownloadInstaller][workflow]")
{
    TestCommon::TempFile testInstallerPath("TestInstaller.txt");

    std::ofstream ofile(testInstallerPath, std::ofstream::out);
    ofile << "test";
    ofile.close();

    ApplyMotwIfApplicable(testInstallerPath, URLZONE_INTERNET);
    VerifyMotw(testInstallerPath, 3);

    std::ostringstream updateMotwOutput;
    TestContext context{ updateMotwOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Add<Data::DownloadHashInfo>({ {}, {} });
    context.Add<Data::InstallerPath>(testInstallerPath);
    auto packageVersion = std::make_shared<TestPackageVersion>(Manifest{});
    auto testSource = std::make_shared<TestSource>();
    testSource->Details.TrustLevel = SourceTrustLevel::Trusted;
    packageVersion->Source = testSource;
    context.Add<Data::PackageVersion>(packageVersion);
    ManifestInstaller installer;
    installer.Url = "http://NotTrusted.com";
    context.Add<Data::Installer>(std::move(installer));

    context << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
    REQUIRE(WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerTrusted));
    VerifyMotw(testInstallerPath, 2);

    testSource->Details.TrustLevel = SourceTrustLevel::None;
    context.ClearFlags(ContextFlag::InstallerTrusted);
    context << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
    REQUIRE_FALSE(WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerTrusted));
    VerifyMotw(testInstallerPath, 3);

    INFO(updateMotwOutput.str());
}

TEST_CASE("ValidateCommand_Dependencies", "[workflow][dependencies]")
{
    std::ostringstream validateOutput;
    TestContext context{ validateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Args::Type::ValidateManifest, TestDataFile("Manifest-Good-AllDependencyTypes.yaml").GetPath().u8string());

    ValidateCommand validate({});
    validate.Execute(context);
    INFO(validateOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(validateOutput.str().find(Resource::LocString(Resource::String::ValidateCommandReportDependencies).get()) != std::string::npos);
    REQUIRE(validateOutput.str().find("WindowsFeaturesDep") != std::string::npos);
    REQUIRE(validateOutput.str().find("WindowsLibrariesDep") != std::string::npos);
    // PackageDep1 has minimum version (1.0), PackageDep2 doesn't (shouldn't show [>=...])
    REQUIRE(validateOutput.str().find("Package.Dep1-x64 [>= 1.0]") != std::string::npos);
    REQUIRE(validateOutput.str().find("Package.Dep2-x64") != std::string::npos);
    REQUIRE(validateOutput.str().find("Package.Dep2-x64 [") == std::string::npos);
    REQUIRE(validateOutput.str().find("ExternalDep") != std::string::npos);
}

TEST_CASE("AdminSetting_LocalManifestFiles", "[LocalManifests][workflow]")
{
    RemoveSetting(Stream::AdminSettings);

    {
        // If there's no admin setting, using local manifest should fail.
        Execution::Args args;
        args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand({});
        REQUIRE_THROWS(installCommand.ValidateArguments(args));
    }

    {
        // Using settings command to enable local manifests
        std::ostringstream settingsOutput;
        TestContext context{ settingsOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(Execution::Args::Type::AdminSettingEnable, "LocalManifestFiles"sv);
        context.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings({});
        settings.Execute(context);
        INFO(settingsOutput.str());
    }

    {
        // Now using local manifests should succeed
        Execution::Args args2;
        args2.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand2({});
        REQUIRE_NOTHROW(installCommand2.ValidateArguments(args2));
    }

    {
        // Using settings command to disable local manifests
        std::ostringstream settingsOutput2;
        TestContext context2{ settingsOutput2, std::cin };
        auto previousThreadGlobals = context2.SetForCurrentThread();
        context2.Args.AddArg(Execution::Args::Type::AdminSettingDisable, "LocalManifestFiles"sv);
        context2.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings2({});
        settings2.Execute(context2);
        INFO(settingsOutput2.str());
    }

    {
        // Now using local manifests should fail
        Execution::Args args3;
        args3.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand3({});
        REQUIRE_THROWS(installCommand3.ValidateArguments(args3));
    }
}

TEST_CASE("Export_Settings", "[Settings][workflow]")
{
    RemoveSetting(Stream::AdminSettings);

    {
        // No admin settings, local manifest should be false.
        std::ostringstream exportOutput;
        TestContext context{ exportOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        SettingsExportCommand settingsExportCommand({});
        settingsExportCommand.Execute(context);

        auto json = ConvertToJson(exportOutput.str());
        REQUIRE(!json.isNull());
        REQUIRE_FALSE(json["adminSettings"]["LocalManifestFiles"].asBool());

        auto userSettingsFileValue = std::string(json["userSettingsFile"].asCString());
        REQUIRE(userSettingsFileValue.find("settings.json") != std::string::npos);
        REQUIRE(json["adminSettingsFile"].asString() == UserSettings::AdminSettingsFilePath().u8string());
    }

    {
        // Enable local manifest and verify export works.
        std::ostringstream settingsOutput;
        TestContext context{ settingsOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(Execution::Args::Type::AdminSettingEnable, "LocalManifestFiles"sv);
        context.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings({});
        settings.Execute(context);

        std::ostringstream exportOutput;
        TestContext context2{ exportOutput, std::cin };
        auto previousThreadGlobals2 = context2.SetForCurrentThread();
        SettingsExportCommand settingsExportCommand({});
        settingsExportCommand.Execute(context2);
        auto json = ConvertToJson(exportOutput.str());
        REQUIRE(!json.isNull());
        REQUIRE(json["adminSettings"]["LocalManifestFiles"].asBool());

        auto userSettingsFileValue = std::string(json["userSettingsFile"].asCString());
        REQUIRE(userSettingsFileValue.find("settings.json") != std::string::npos);
        REQUIRE(json["adminSettingsFile"].asString() == UserSettings::AdminSettingsFilePath().u8string());
    }
}

TEST_CASE("Settings_AdminFileArguments", "[Settings][workflow]")
{
    SettingsCommand command({});
    Args args;
    args.AddArg(Args::Type::AdminSettingsFile);
    REQUIRE_NOTHROW(command.ValidateArguments(args));
    args.AddArg(Args::Type::CreateAdminSettingsFile);
    REQUIRE_NOTHROW(command.ValidateArguments(args));

    SECTION("Enable is not a file operation")
    {
        args.AddArg(Args::Type::AdminSettingEnable, "LocalManifestFiles"sv);
        REQUIRE_THROWS(command.ValidateArguments(args));
    }
    SECTION("Disable is not a file operation")
    {
        args.AddArg(Args::Type::AdminSettingDisable, "LocalManifestFiles"sv);
        REQUIRE_THROWS(command.ValidateArguments(args));
    }
    SECTION("Create requires admin target")
    {
        Args createOnly;
        createOnly.AddArg(Args::Type::CreateAdminSettingsFile);
        REQUIRE_THROWS(command.ValidateArguments(createOnly));
    }
    SECTION("Create does not open the editor")
    {
        std::ostringstream output;
        TestContext context{ output, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(Args::Type::AdminSettingsFile);
        context.Args.AddArg(Args::Type::CreateAdminSettingsFile);
        context.Override({ EnsureRunningAsAdmin, [](TestContext&) {}, 1 });
        context.Override({ CreateAdminSettingsFile, [](TestContext&) {}, 1 });
        command.Execute(context);
        REQUIRE_FALSE(context.IsTerminated());
    }
}

TEST_CASE("Settings_WarningDisplay", "[Settings][workflow]")
{
    struct SettingsWithWarnings : TestUserSettings
    {
        SettingsWithWarnings() { m_warnings.clear(); }
        using UserSettings::m_warnings;
    } settings;
    UserSettings::Warning field{ AppInstaller::StringResource::String::SettingsWarningInvalidFieldValue, ".visual.progressBar", "invalid" };
    field.Source = LocIndString{ "admin-settings.json" };
    settings.m_warnings.push_back(field);
    UserSettings::Warning migration{ AppInstaller::StringResource::String::SettingsWarningIgnoredWithFullToken, ".downloadBehavior.defaultDownloadDirectory" };
    migration.Source = LocIndString{ "settings.json" };
    migration.IsMigrationWarning = true;
    settings.m_warnings.push_back(migration);

    const auto warningType = GENERATE(SettingsWarningType::Migration, SettingsWarningType::NonMigration);
    const bool migrationOnly = warningType == SettingsWarningType::Migration;
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context << ShowSettingsWarnings{ warningType };
    const auto& expected = migrationOnly ? migration : field;
    const auto& absent = migrationOnly ? field : migration;
    REQUIRE(output.str().find(expected.Message(expected.Path, expected.Data, expected.Source).get()) != std::string::npos);
    REQUIRE(output.str().find(absent.Path.get()) == std::string::npos);
    REQUIRE(output.str().find("{0}") == std::string::npos);
    REQUIRE(output.str().find("{1}") == std::string::npos);
    REQUIRE(output.str().find("{2}") == std::string::npos);

    SECTION("Shared command reporting does not duplicate migration warnings")
    {
        std::ostringstream commandOutput;
        TestContext commandContext{ commandOutput, std::cin };
        auto commandThreadGlobals = commandContext.SetForCurrentThread();
        commandContext.Override({ OpenUserSetting, [](TestContext& ctx) { ctx << ShowSettingsWarnings{ SettingsWarningType::NonMigration }; }, 1 });
        SettingsCommand command({});
        ExecuteWithoutLoggingSuccess(commandContext, &command);
        REQUIRE_FALSE(commandContext.IsTerminated());
        const auto text = migration.Message(migration.Path, migration.Data, migration.Source);
        const auto position = commandOutput.str().find(text.get());
        REQUIRE(position != std::string::npos);
        REQUIRE(commandOutput.str().find(text.get(), position + text.get().size()) == std::string::npos);
        REQUIRE(commandOutput.str().find(field.Path.get()) != std::string::npos);
    }

    SECTION("Machine-readable export suppresses warnings")
    {
        std::ostringstream exportOutput;
        TestContext exportContext{ exportOutput, std::cin };
        auto exportThreadGlobals = exportContext.SetForCurrentThread();
        SettingsExportCommand command({});
        ExecuteWithoutLoggingSuccess(exportContext, &command);
        REQUIRE_FALSE(exportContext.IsTerminated());
        auto json = ConvertToJson(exportOutput.str());
        REQUIRE(json["adminSettingsFile"].asString() == UserSettings::AdminSettingsFilePath().u8string());
        REQUIRE(exportOutput.str().find(migration.Path.get()) == std::string::npos);
    }
}

TEST_CASE("Settings_AdminFileMissingReadOnly", "[Settings][workflow]")
{
    if (AppInstaller::Runtime::IsRunningAsAdminOrSystem())
    {
        WARN("Test requires a non-administrator token. Skipped.");
        return;
    }

    auto files = DeleteUserSettingsFiles();
    TestUserSettings settings;
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Args::Type::AdminSettingsFile);
    OpenUserSetting(context);

    REQUIRE_TERMINATED_WITH(context, E_ACCESSDENIED);
    REQUIRE(output.str().find(UserSettings::AdminSettingsFilePath().u8string()) != std::string::npos);
    REQUIRE_FALSE(std::filesystem::exists(UserSettings::AdminSettingsFilePath()));
    REQUIRE_FALSE(std::filesystem::exists(GetPathTo(Stream::BackupAdminSettingsFile)));
}

TEST_CASE("Settings_DscUserSettingsFileTarget", "[Settings][workflow]")
{
    auto files = DeleteUserSettingsFiles();
    const bool admin = GENERATE(false, true);
    const std::string action = GENERATE("Partial", "Full");
    CAPTURE(admin, action);
    auto ordinary = ConvertToJson(R"({ "file": "ordinary", "retained": true, "array": [1, 2] })");
    auto protectedSettings = ConvertToJson(R"({ "file": "protected", "retained": true, "array": [1, 2] })");
    SetSetting(Stream::PrimaryUserSettings, Json::writeString(Json::StreamWriterBuilder{}, ordinary));
    {
        const auto path = UserSettings::AdminSettingsFilePath();
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file{ path, std::ios::binary };
        file << Json::writeString(Json::StreamWriterBuilder{}, protectedSettings);
        file.close();
        REQUIRE_FALSE(file.fail());
    }

    Json::Value input{ Json::ValueType::objectValue };
    if (admin)
    {
        input["admin"] = true;
    }
    auto invoke = [&](Args::Type function)
    {
        std::istringstream inputStream{ Json::writeString(Json::StreamWriterBuilder{}, input) };
        std::ostringstream output;
        TestContext context{ output, inputStream };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(function);
        DscUserSettingsFileResource command({});
        command.Execute(context);
        INFO(output.str());
        REQUIRE(context.GetTerminationHR() == S_OK);

        std::vector<Json::Value> result;
        std::istringstream outputStream{ output.str() };
        for (std::string line; std::getline(outputStream, line);)
        {
            result.emplace_back(ConvertToJson(line));
        }
        REQUIRE_FALSE(result.empty());
        return result;
    };

    auto expected = admin ? protectedSettings : ordinary;
    for (auto function : { Args::Type::DscResourceFunctionGet, Args::Type::DscResourceFunctionExport })
    {
        auto result = invoke(function);
        REQUIRE(result.size() == 1);
        REQUIRE(result[0]["admin"].isBool());
        REQUIRE(result[0]["admin"].asBool() == admin);
        REQUIRE(result[0]["settings"] == expected);
    }

    auto schema = invoke(Args::Type::DscResourceFunctionSchema)[0];
    REQUIRE(schema["properties"]["admin"]["type"].asString() == "boolean");
    REQUIRE(schema["properties"]["admin"]["default"] == Json::Value{ false });

    input["action"] = action;
    input["settings"] = expected;
    auto result = invoke(Args::Type::DscResourceFunctionTest);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0]["admin"].asBool() == admin);
    REQUIRE(result[0]["_inDesiredState"].asBool());
    REQUIRE(result[1].empty());

    input["settings"] = ConvertToJson(R"({ "array": [3], "downloadBehavior": { "defaultDownloadDirectory": null } })");
    result = invoke(Args::Type::DscResourceFunctionTest);
    REQUIRE(result.size() == 2);
    REQUIRE_FALSE(result[0]["_inDesiredState"].asBool());
    REQUIRE(result[1][0].asString() == "settings");
    REQUIRE_FALSE(Stream{ Stream::BackupAdminSettingsFile }.Get());

    if (admin && !AppInstaller::Runtime::IsRunningAsAdminOrSystem())
    {
        WARN("Protected Set requires admin privilege. Read-only operations were checked.");
        return;
    }

    const bool createFile = GENERATE(false, true);
    CAPTURE(createFile);
    if (createFile)
    {
        const auto path = admin ? UserSettings::AdminSettingsFilePath() : UserSettings::SettingsFilePath();
        REQUIRE(std::filesystem::remove(path));
        result = invoke(Args::Type::DscResourceFunctionGet);
        REQUIRE(result[0]["settings"] == Json::Value{ Json::ValueType::objectValue });
        REQUIRE_FALSE(std::filesystem::exists(path));
    }

    if (createFile || action == "Full")
    {
        expected = input["settings"];
    }
    else
    {
        expected["array"] = input["settings"]["array"];
        expected["downloadBehavior"] = input["settings"]["downloadBehavior"];
    }
    result = invoke(Args::Type::DscResourceFunctionSet);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0]["admin"].asBool() == admin);
    REQUIRE(result[0]["settings"] == expected);
    REQUIRE(result[1][0].asString() == "settings");
    result = invoke(Args::Type::DscResourceFunctionTest);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0]["_inDesiredState"].asBool());
    REQUIRE(result[1].empty());
    REQUIRE_FALSE(Stream{ Stream::BackupAdminSettingsFile }.Get());

    auto selected = Stream{ admin ? Stream::PrimaryAdminSettingsFile : Stream::PrimaryUserSettings }.Get();
    REQUIRE(selected);
    REQUIRE(ConvertToJson(ReadEntireStream(*selected)) == expected);
    auto unselected = Stream{ admin ? Stream::PrimaryUserSettings : Stream::PrimaryAdminSettingsFile }.Get();
    REQUIRE(unselected);
    REQUIRE(ConvertToJson(ReadEntireStream(*unselected)) == (admin ? ordinary : protectedSettings));
}

TEST_CASE("Settings_DscUserSettingsFileAdminRequiresElevation", "[Settings][workflow]")
{
    auto files = DeleteUserSettingsFiles();
    std::istringstream input{ R"({ "admin": true, "settings": {} })" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Args::Type::DscResourceFunctionSet);
    context.Override({ EnsureRunningAsAdmin, [](TestContext& ctx)
    {
        AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN);
    }, 1 });
    DscUserSettingsFileResource command({});
    command.Execute(context);

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_COMMAND_REQUIRES_ADMIN);
    REQUIRE_FALSE(std::filesystem::exists(UserSettings::AdminSettingsFilePath()));
    REQUIRE_FALSE(std::filesystem::exists(UserSettings::SettingsFilePath()));
}
