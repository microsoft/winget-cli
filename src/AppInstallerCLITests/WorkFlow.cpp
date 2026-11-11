// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include <AppInstallerDownloader.h>
#include <winget/Runtime.h>
#include <winget/Settings.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/UpdateFlow.h>
#include <Commands/DownloadCommand.h>
#include <Commands/InstallCommand.h>
#include <Commands/SettingsCommand.h>
#include <Commands/ShowCommand.h>
#include <Commands/ValidateCommand.h>

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
    }
}

namespace
{
    using namespace AppInstaller::Manifest;

    struct InstallerEquivalenceTestSetup
    {
        TestUserSettings Settings;
        std::ostringstream Output;
        std::istringstream Input{ "2\n" };
        TestContext Context{ Output, Input };
        std::vector<Manifest> Manifests;

        InstallerEquivalenceTestSetup(size_t count = 2)
        {
            Manifest manifest;
            manifest.Version = "2.4";
            ManifestInstaller installer;
            installer.BaseInstallerType = InstallerTypeEnum::Exe;
            installer.Arch = AppInstaller::Utility::Architecture::X86;
            installer.Scope = ScopeEnum::User;
            installer.Sha256.assign(32, 0x42);
            manifest.Installers.emplace_back(installer);
            for (size_t i = 0; i < count; ++i)
            {
                manifest.Id = "Contoso.Editor" + std::to_string(i);
                manifest.DefaultLocalization.Add<Localization::PackageName>("Editor " + std::to_string(i));
                manifest.Installers[0].Url = "https://example.com/editor" + std::to_string(i) + ".exe";
                Manifests.emplace_back(manifest);
            }
        }

        auto Compare(OperationType operation = OperationType::Install)
        {
            auto flags = Context.GetFlags();
            auto result = GetEquivalentInstallers(Context, operation, Manifests);
            REQUIRE_FALSE(Context.IsTerminated());
            REQUIRE(Context.GetFlags() == flags);
            REQUIRE_FALSE(Context.Contains(Execution::Data::Package));
            REQUIRE_FALSE(Context.Contains(Execution::Data::PackageVersion));
            REQUIRE_FALSE(Context.Contains(Execution::Data::Manifest));
            REQUIRE_FALSE(Context.Contains(Execution::Data::Installer));
            REQUIRE(Output.str().empty());
            return result;
        }
    };

    struct DeduplicationTestVersion : TestPackageVersion
    {
        using TestPackageVersion::TestPackageVersion;
        std::optional<std::string> SearchVersion;
        std::function<void()> OnManifest;

        LocIndString GetProperty(PackageVersionProperty property) const override
        {
            return property == PackageVersionProperty::Version && SearchVersion ?
                LocIndString{ *SearchVersion } : TestPackageVersion::GetProperty(property);
        }

        Manifest GetManifest() override
        {
            auto manifest = TestPackageVersion::GetManifest();
            if (OnManifest) { OnManifest(); }
            return manifest;
        }
    };

    struct PackageDeduplicationTestSetup : InstallerEquivalenceTestSetup
    {
        std::vector<std::shared_ptr<TestSource>> Sources;
        std::vector<std::shared_ptr<TestCompositePackage>> Packages;
        std::vector<std::shared_ptr<DeduplicationTestVersion>> Versions;

        PackageDeduplicationTestSetup(size_t count = 2) : InstallerEquivalenceTestSetup(count)
        {
            SearchResult result;
            for (size_t i = 0; i < count; ++i)
            {
                auto source = std::make_shared<TestSource>();
                source->Details.Identifier = "Source" + std::to_string(i);
                source->Details.Name = source->Details.Identifier;
                auto package = TestCompositePackage::Make(std::vector<Manifest>{ Manifests[i] }, source);
                auto version = std::make_shared<DeduplicationTestVersion>(Manifests[i], source);
                package->Available[0]->Versions = { version };
                result.Matches.emplace_back(package, PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, Manifests[i].Id });
                Sources.emplace_back(source);
                Packages.emplace_back(package);
                Versions.emplace_back(version);
            }
            Context.Add<Execution::Data::SearchResult>(std::move(result));
            SetSourceOrder();
        }

        void SetSourceOrder(bool reverse = false)
        {
            std::vector<Source> sources;
            for (const auto& source : Sources) { sources.emplace_back(source); }
            if (reverse) { std::reverse(sources.begin(), sources.end()); }
            Context.Add<Execution::Data::Source>(Source{ sources });
        }

        auto Prepare(OperationType operation = OperationType::Install)
        {
            auto flags = Context.GetFlags();
            auto matches = Context.Get<Execution::Data::SearchResult>().Matches;
            auto result = PreparePackageDeduplication(Context, operation);
            REQUIRE_FALSE(Context.IsTerminated());
            REQUIRE(Context.GetFlags() == flags);
            REQUIRE_FALSE(Context.Contains(Execution::Data::Package));
            REQUIRE_FALSE(Context.Contains(Execution::Data::PackageVersion));
            REQUIRE_FALSE(Context.Contains(Execution::Data::Manifest));
            REQUIRE_FALSE(Context.Contains(Execution::Data::Installer));
            REQUIRE(Output.str().empty());
            const auto& remaining = Context.Get<Execution::Data::SearchResult>().Matches;
            REQUIRE(remaining.size() == matches.size());
            for (size_t i = 0; i < matches.size(); ++i) { REQUIRE(remaining[i].Package == matches[i].Package); }
            return result;
        }

        void RequireManifestCalls(size_t count)
        {
            for (const auto& source : Sources) { REQUIRE(source->CountOfCallsRequiringManifestData == count); }
        }
    };
}

TEST_CASE("PackageDeduplication_EquivalentInstallers", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto count = GENERATE(size_t{ 2 }, size_t{ 3 });
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    InstallerEquivalenceTestSetup setup{ count };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Manifests[0].Channel = "Stable";

    for (size_t i = 1; i < count; ++i)
    {
        setup.Manifests[i].Version = "2.4.0";
        setup.Manifests[i].Channel = "stable";
        auto& installer = setup.Manifests[i].Installers[0];
        installer.Switches[InstallerSwitchType::Silent] = "/different-silent";
        installer.Switches[InstallerSwitchType::Update] = "/different-update";
        installer.Switches[InstallerSwitchType::Log] = "/different-log";
        installer.InstallerSuccessCodes = { 42 };
        installer.ExpectedReturnCodes[43] = { ExpectedReturnCodeEnum::RebootRequiredToFinish, "https://example.com/help" };
        installer.AuthInfo.Type = AppInstaller::Authentication::AuthenticationType::MicrosoftEntraId;
        installer.AuthInfo.MicrosoftEntraIdInfo = AppInstaller::Authentication::MicrosoftEntraIdAuthenticationInfo{ "https://example.com", "scope" };
    }

    auto result = setup.Compare(operation);
    REQUIRE(result);
    REQUIRE(result->size() == count);
    REQUIRE(setup.Manifests[0].Version == "2.4");
    REQUIRE(setup.Manifests[1].Version == "2.4.0");
    for (size_t i = 0; i < count; ++i)
    {
        const auto& expected = setup.Manifests[i].Installers[0];
        REQUIRE(result->at(i).Url == expected.Url);
        REQUIRE(result->at(i).Sha256 == expected.Sha256);
        REQUIRE(result->at(i).Switches == expected.Switches);
        REQUIRE(result->at(i).InstallerSuccessCodes == expected.InstallerSuccessCodes);
        REQUIRE(result->at(i).ExpectedReturnCodes.size() == expected.ExpectedReturnCodes.size());
        REQUIRE(result->at(i).AuthInfo.Type == expected.AuthInfo.Type);
        if (i > 0)
        {
            REQUIRE(result->at(i).ExpectedReturnCodes.at(43).ReturnResponseEnum == expected.ExpectedReturnCodes.at(43).ReturnResponseEnum);
            REQUIRE(result->at(i).ExpectedReturnCodes.at(43).ReturnResponseUrl == expected.ExpectedReturnCodes.at(43).ReturnResponseUrl);
            REQUIRE(result->at(i).AuthInfo.MicrosoftEntraIdInfo);
            REQUIRE(result->at(i).AuthInfo.MicrosoftEntraIdInfo->Resource == expected.AuthInfo.MicrosoftEntraIdInfo->Resource);
            REQUIRE(result->at(i).AuthInfo.MicrosoftEntraIdInfo->Scope == expected.AuthInfo.MicrosoftEntraIdInfo->Scope);
        }
    }
}

TEST_CASE("PackageDeduplication_InvalidResolvedVersion", "[PackageDeduplication][workflow]")
{
    auto version = GENERATE("", " ", "Unknown", "unknown", "Latest", "latest", "< 2.4", "> 2.4", "< Unknown", "> Unknown");
    CAPTURE(version);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    for (auto& manifest : setup.Manifests)
    {
        manifest.Version = std::string{ version };
    }
    REQUIRE_FALSE(setup.Compare());
}

TEST_CASE("PackageDeduplication_IdentityMismatch", "[PackageDeduplication][workflow]")
{
    auto index = GENERATE(size_t{ 0 }, size_t{ 1 }, size_t{ 2 });
    InstallerEquivalenceTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    auto& manifest = setup.Manifests[index];
    SECTION("Version") { manifest.Version = "2.5"; }
    SECTION("Channel") { manifest.Channel = "Preview"; }
    SECTION("Last hash byte") { manifest.Installers[0].Sha256.back() ^= 1; }
    REQUIRE_FALSE(setup.Compare());
}

TEST_CASE("PackageDeduplication_InvalidHash", "[PackageDeduplication][workflow]")
{
    auto length = GENERATE(size_t{ 0 }, size_t{ 31 }, size_t{ 32 }, size_t{ 33 });
    auto value = GENERATE(BYTE{ 0 }, BYTE{ 0x42 });
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    for (auto& manifest : setup.Manifests)
    {
        manifest.Installers[0].Sha256.assign(length, value);
    }
    REQUIRE(setup.Compare().has_value() == (length == 32 && value != 0));
}

TEST_CASE("PackageDeduplication_ExecutionFamily", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto first = GENERATE(InstallerTypeEnum::Exe, InstallerTypeEnum::Burn, InstallerTypeEnum::Inno, InstallerTypeEnum::Nullsoft,
        InstallerTypeEnum::Msi, InstallerTypeEnum::Wix, InstallerTypeEnum::Msix);
    auto second = GENERATE(InstallerTypeEnum::Exe, InstallerTypeEnum::Burn, InstallerTypeEnum::Inno, InstallerTypeEnum::Nullsoft,
        InstallerTypeEnum::Msi, InstallerTypeEnum::Wix, InstallerTypeEnum::Msix);
    CAPTURE(first, second);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Manifests[0].Installers[0].BaseInstallerType = first;
    setup.Manifests[1].Installers[0].BaseInstallerType = second;
    auto family = [](InstallerTypeEnum type)
    {
        if (type == InstallerTypeEnum::Msix) { return 0; }
        if (type == InstallerTypeEnum::Msi || type == InstallerTypeEnum::Wix) { return 1; }
        return 2;
    };
    REQUIRE(setup.Compare().has_value() == (family(first) == family(second)));
}

TEST_CASE("PackageDeduplication_UnsupportedInstaller", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto type = GENERATE(InstallerTypeEnum::Unknown, InstallerTypeEnum::MSStore, InstallerTypeEnum::Zip,
        InstallerTypeEnum::Portable, InstallerTypeEnum::Font, InstallerTypeEnum::Exe);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    for (auto& manifest : setup.Manifests)
    {
        manifest.Installers[0].BaseInstallerType = type;
        manifest.Installers[0].NestedInstallerType = InstallerTypeEnum::Exe;
    }
    REQUIRE_FALSE(setup.Compare());
}

TEST_CASE("PackageDeduplication_WholeSetScope", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto first = GENERATE(ScopeEnum::Unknown, ScopeEnum::User, ScopeEnum::Machine);
    auto second = GENERATE(ScopeEnum::Unknown, ScopeEnum::User, ScopeEnum::Machine);
    auto third = GENERATE(ScopeEnum::Unknown, ScopeEnum::User, ScopeEnum::Machine);
    auto type = GENERATE(InstallerTypeEnum::Exe, InstallerTypeEnum::Msix);
    CAPTURE(first, second, third, type);
    InstallerEquivalenceTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Manifests[0].Installers[0].Scope = first;
    setup.Manifests[1].Installers[0].Scope = second;
    setup.Manifests[2].Installers[0].Scope = third;
    for (auto& manifest : setup.Manifests)
    {
        manifest.Installers[0].BaseInstallerType = type;
    }
    bool user = first == ScopeEnum::User || second == ScopeEnum::User || third == ScopeEnum::User;
    bool machine = first == ScopeEnum::Machine || second == ScopeEnum::Machine || third == ScopeEnum::Machine;
    REQUIRE(setup.Compare().has_value() == (type == InstallerTypeEnum::Msix || !user || !machine));
}

TEST_CASE("PackageDeduplication_InstallerApplicability", "[PackageDeduplication][workflow]")
{
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    auto& installer = setup.Manifests[1].Installers[0];
    SECTION("No installers") { setup.Manifests[1].Installers.clear(); }
    SECTION("OS version") { installer.MinOSVersion = "99.0.0.0"; }
    SECTION("Architecture")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::InstallArchitecture, "x86"sv);
        installer.Arch = AppInstaller::Utility::Architecture::X64;
    }
    SECTION("Scope")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::InstallScope, "user"sv);
        installer.Scope = AppInstaller::Manifest::ScopeEnum::Machine;
    }
    SECTION("Locale")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
        setup.Manifests[0].Installers[0].Locale = "en-US";
        installer.Locale = "fr-FR";
    }
    SECTION("Type")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::InstallerType, "exe"sv);
        installer.BaseInstallerType = AppInstaller::Manifest::InstallerTypeEnum::Msi;
    }
    REQUIRE_FALSE(setup.Compare());
}

TEST_CASE("PackageDeduplication_OnlyPreferredInstaller", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    bool reverse = GENERATE(false, true);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::InstallScopePreference>(ScopeEnum::User);
    auto& installers = setup.Manifests[1].Installers;
    auto alternate = installers[0];
    alternate.Scope = ScopeEnum::Machine;
    installers[0].Sha256.back() ^= 1;
    installers.emplace_back(alternate);
    if (reverse)
    {
        std::reverse(installers.begin(), installers.end());
    }
    REQUIRE_FALSE(setup.Compare());
}

TEST_CASE("PackageDeduplication_ApplicableMetadataDifferences", "[PackageDeduplication][workflow]")
{
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Manifests[0].Installers[0].Locale = "en-US";
    setup.Manifests[1].Installers[0].Locale = "fr-FR";
    setup.Manifests[1].Installers[0].Arch = AppInstaller::Utility::Architecture::Neutral;
    auto result = setup.Compare();
    REQUIRE(result);
    REQUIRE(result->at(0).Locale == "en-US");
    REQUIRE(result->at(1).Locale == "fr-FR");
    REQUIRE(result->at(0).Arch == AppInstaller::Utility::Architecture::X86);
    REQUIRE(result->at(1).Arch == AppInstaller::Utility::Architecture::Neutral);
}

TEST_CASE("PackageDeduplication_DependencySideEffects", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    auto type = GENERATE(DependencyType::Package, DependencyType::WindowsFeature, DependencyType::WindowsLibrary, DependencyType::External);
    bool skipArg = GENERATE(false, true);
    bool skipSetting = GENERATE(false, true);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::InstallSkipDependencies>(bool{ skipSetting });
    if (skipArg) { setup.Context.Args.AddArg(Execution::Args::Type::SkipDependencies); }
    setup.Manifests[1].Installers[0].Dependencies.Add(Dependency{ type, "Contoso.Dependency" });
    bool processes = !skipArg && !skipSetting && operation != OperationType::Show;
    bool blocked = processes && (type == DependencyType::Package || (type == DependencyType::WindowsFeature && operation == OperationType::Install));
    REQUIRE(setup.Compare(operation).has_value() == !blocked);
}

TEST_CASE("PackageDeduplication_MatchingDependencies", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    bool expected = true;
    SECTION("Feature order and case")
    {
        setup.Manifests[0].Installers[0].Dependencies.Add(Dependency{ DependencyType::WindowsFeature, "FeatureA" });
        setup.Manifests[0].Installers[0].Dependencies.Add(Dependency{ DependencyType::WindowsFeature, "FeatureB" });
        setup.Manifests[1].Installers[0].Dependencies.Add(Dependency{ DependencyType::WindowsFeature, "featureb" });
        setup.Manifests[1].Installers[0].Dependencies.Add(Dependency{ DependencyType::WindowsFeature, "FEATUREA" });
    }
    SECTION("Identical package IDs still depend on their sources")
    {
        for (auto& manifest : setup.Manifests)
        {
            manifest.Installers[0].Dependencies.Add(Dependency{ DependencyType::Package, "Contoso.Dependency" });
        }
        expected = false;
    }
    SECTION("Different feature IDs with the same count")
    {
        setup.Manifests[0].Installers[0].Dependencies.Add(Dependency{ DependencyType::WindowsFeature, "FeatureA" });
        setup.Manifests[1].Installers[0].Dependencies.Add(Dependency{ DependencyType::WindowsFeature, "FeatureB" });
        expected = false;
    }
    REQUIRE(setup.Compare().has_value() == expected);
}

TEST_CASE("PackageDeduplication_OperationRestrictions", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    auto& installer = setup.Manifests[1].Installers[0];
    bool expected = true;
    SECTION("Download prohibited")
    {
        installer.DownloadCommandProhibited = true;
        expected = operation != OperationType::Download;
    }
    SECTION("Elevation prohibited")
    {
        installer.ElevationRequirement = ElevationRequirementEnum::ElevationProhibited;
        expected = operation != OperationType::Install || !AppInstaller::Runtime::IsRunningAsAdmin();
    }
    REQUIRE(setup.Compare(operation).has_value() == expected);
}

TEST_CASE("PackageDeduplication_UnsupportedArguments", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    auto argument = GENERATE(UnsupportedArgumentEnum::Location, UnsupportedArgumentEnum::Log);
    bool supplied = GENERATE(false, true);
    bool hasCommand = GENERATE(false, true);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    InstallCommand command{ {} };
    if (hasCommand) { setup.Context.SetExecutingCommand(&command); }
    setup.Manifests[1].Installers[0].UnsupportedArguments.emplace_back(argument);
    if (supplied)
    {
        setup.Context.Args.AddArg(argument == UnsupportedArgumentEnum::Location ?
            Execution::Args::Type::InstallLocation : Execution::Args::Type::Log, "C:\\unused"sv);
    }
    bool blocked = operation == OperationType::Install && argument == UnsupportedArgumentEnum::Location && supplied && hasCommand;
    REQUIRE(setup.Compare(operation).has_value() == !blocked);
    setup.Context.Add<Execution::Data::Installer>(setup.Manifests[1].Installers[0]);
    setup.Context << CheckForUnsupportedArgs;
    bool installBlocked = argument == UnsupportedArgumentEnum::Location && supplied && hasCommand;
    REQUIRE(setup.Context.GetTerminationHR() == (installBlocked ? APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT : S_OK));
}

TEST_CASE("PackageDeduplication_PreservesExistingContext", "[PackageDeduplication][workflow]")
{
    bool equivalent = GENERATE(false, true);
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    auto existingManifest = setup.Manifests[0];
    existingManifest.Id = "Contoso.Unrelated";
    existingManifest.Version = "1.0";
    setup.Context.Add<Execution::Data::Manifest>(existingManifest);
    setup.Context.Add<Execution::Data::Installer>(existingManifest.Installers[0]);
    if (!equivalent)
    {
        setup.Manifests[1].Installers[0].Sha256.back() ^= 1;
    }
    REQUIRE(GetEquivalentInstallers(setup.Context, OperationType::Install, setup.Manifests).has_value() == equivalent);
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Id == existingManifest.Id);
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Version == existingManifest.Version);
    REQUIRE(setup.Context.Get<Execution::Data::Installer>()->Url == existingManifest.Installers[0].Url);
    REQUIRE(setup.Context.Get<Execution::Data::Installer>()->Sha256 == existingManifest.Installers[0].Sha256);
}

TEST_CASE("PackageDeduplication_ComparisonBounds", "[PackageDeduplication][workflow]")
{
    auto count = GENERATE(size_t{ 0 }, size_t{ 1 }, size_t{ 2 }, size_t{ 3 }, size_t{ 4 });
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show, OperationType::Upgrade,
        OperationType::Uninstall, OperationType::Repair, OperationType::Export, OperationType::Pin, OperationType::List,
        OperationType::Search, OperationType::Completion);
    InstallerEquivalenceTestSetup setup{ count };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    bool supported = operation == OperationType::Install || operation == OperationType::Download || operation == OperationType::Show;
    REQUIRE(setup.Compare(operation).has_value() == (supported && count >= 2 && count <= 3));
}

TEST_CASE("PackageDeduplication_PrepareSourceOrder", "[PackageDeduplication][workflow]")
{
    auto count = GENERATE(size_t{ 2 }, size_t{ 3 });
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    bool reverseResults = GENERATE(false, true);
    bool reverseSources = GENERATE(false, true);
    PackageDeduplicationTestSetup setup{ count };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    if (reverseResults)
    {
        auto& matches = setup.Context.Get<Execution::Data::SearchResult>().Matches;
        std::reverse(matches.begin(), matches.end());
    }
    setup.SetSourceOrder(reverseSources);
    auto result = setup.Prepare(operation);
    size_t selected = reverseSources ? count - 1 : 0;
    REQUIRE(result);
    REQUIRE(result->Operation == operation);
    REQUIRE(result->Package == setup.Packages[selected]);
    REQUIRE(result->PackageVersion == setup.Versions[selected]);
    REQUIRE(result->Manifest.Id == setup.Manifests[selected].Id);
    REQUIRE(result->Installer.Url == setup.Manifests[selected].Installers[0].Url);
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_PrepareBounds", "[PackageDeduplication][workflow]")
{
    auto count = GENERATE(size_t{ 0 }, size_t{ 1 }, size_t{ 2 }, size_t{ 3 }, size_t{ 4 });
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show, OperationType::Upgrade,
        OperationType::Uninstall, OperationType::Repair, OperationType::Export, OperationType::Pin, OperationType::List,
        OperationType::Search, OperationType::Completion);
    PackageDeduplicationTestSetup setup{ count };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    bool supported = (count == 2 || count == 3) &&
        (operation == OperationType::Install || operation == OperationType::Download || operation == OperationType::Show);
    REQUIRE(setup.Prepare(operation).has_value() == supported);
    setup.RequireManifestCalls(supported ? size_t{ 1 } : size_t{ 0 });
}

TEST_CASE("PackageDeduplication_PrepareGuardsBeforeRetrieval", "[PackageDeduplication][workflow]")
{
    auto index = GENERATE(size_t{ 0 }, size_t{ 1 }, size_t{ 2 });
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    auto& result = setup.Context.Get<Execution::Data::SearchResult>();
    auto& package = setup.Packages[index];
    SECTION("Truncated") { result.Truncated = true; }
    SECTION("Search failure") { result.Failures.push_back({ "FailedSource", std::make_exception_ptr(std::runtime_error("failed")) }); }
    SECTION("Null package") { result.Matches[index].Package.reset(); }
    SECTION("Installed association") { package->Installed = TestPackage::Make(setup.Manifests[index], TestPackage::MetadataMap{}); }
    SECTION("Empty installed association") { package->Installed = TestPackage::Make(std::vector<AppInstaller::Manifest::Manifest>{}); }
    SECTION("No available identity") { package->Available.clear(); }
    SECTION("Null available identity") { package->Available[0].reset(); }
    SECTION("Composite identity") { package->Available.emplace_back(setup.Packages[(index + 1) % 3]->Available[0]); }
    SECTION("Repeated candidate source") { package->Available[0]->Source = setup.Sources[(index + 1) % 3]; }
    SECTION("No candidate source") { package->Available[0]->Source.reset(); }
    SECTION("Empty source ID") { setup.Sources[index]->Details.Identifier.clear(); }
    SECTION("Repeated opened source") { setup.Sources[index]->Details.Identifier = setup.Sources[(index + 1) % 3]->Details.Identifier; }
    SECTION("Unopened source")
    {
        setup.Context.Add<Execution::Data::Source>(Source{ std::vector<Source>{ Source{ setup.Sources[(index + 1) % 3] }, Source{ setup.Sources[(index + 2) % 3] } } });
    }
    SECTION("Different priorities") { setup.Sources[index]->Details.Priority = 10; }
    SECTION("Noncomposite source") { setup.Context.Add<Execution::Data::Source>(Source{ setup.Sources[0] }); }
    SECTION("Empty source") { setup.Context.Add<Execution::Data::Source>(Source{}); }
    SECTION("Local manifest") { setup.Context.Args.AddArg(Execution::Args::Type::Manifest, "unused.yaml"sv); }
    SECTION("Show versions") { setup.Context.Args.AddArg(Execution::Args::Type::ListVersions); }
    SECTION("Multiple queries")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::MultiQuery, "first"sv);
        setup.Context.Args.AddArg(Execution::Args::Type::MultiQuery, "second"sv);
    }
    SECTION("Multiple packages") { setup.Context.Add<Execution::Data::PackageSubContexts>(std::vector<std::unique_ptr<Execution::Context>>{}); }
    SECTION("Dependency source") { setup.Context.Add<Execution::Data::DependencySource>(Source{ setup.Sources[0] }); }
    SECTION("Upgrade context") { setup.Context.SetFlags(Execution::ContextFlag::InstallerExecutionUseUpdate); }
    SECTION("Repair context") { setup.Context.SetFlags(Execution::ContextFlag::InstallerExecutionUseRepair); }
    SECTION("Installed version in context") { setup.Context.Add<Execution::Data::InstalledPackageVersion>(setup.Versions[0]); }
    REQUIRE_FALSE(setup.Prepare());
    setup.RequireManifestCalls(0);
    for (const auto& source : setup.Sources) { REQUIRE(source->CountOfCallsRequiringVersionData == 0); }
}

TEST_CASE("PackageDeduplication_PrepareMissingInput", "[PackageDeduplication][workflow]")
{
    InstallerEquivalenceTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    SECTION("No source or results") {}
    SECTION("No source") { setup.Context.Add<Execution::Data::SearchResult>(SearchResult{}); }
    SECTION("No results") { setup.Context.Add<Execution::Data::Source>(Source{}); }
    REQUIRE_FALSE(PreparePackageDeduplication(setup.Context, OperationType::Install));
    REQUIRE_FALSE(setup.Context.IsTerminated());
}

TEST_CASE("PackageDeduplication_PrepareTargetVersion", "[PackageDeduplication][workflow]")
{
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    bool expected = true;
    std::string expectedVersion = "2.4";
    size_t firstManifestCalls = 1;
    size_t secondManifestCalls = 1;
    for (size_t i = 0; i < setup.Packages.size(); ++i)
    {
        auto older = setup.Manifests[i];
        older.Version = "1.0";
        older.Channel = "Preview";
        setup.Packages[i]->Available[0]->Versions.emplace_back(TestPackageVersion::Make(older, setup.Sources[i]));
    }
    SECTION("Latest") {}
    SECTION("Explicit version")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::Version, "1.0"sv);
        expectedVersion = "1.0";
    }
    SECTION("Explicit version and channel")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::Version, "1.0"sv);
        setup.Context.Args.AddArg(Execution::Args::Type::Channel, "Preview"sv);
        expectedVersion = "1.0";
    }
    SECTION("Channel only follows existing command rules")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::Channel, "Preview"sv);
        expectedVersion = operation == OperationType::Install ? "2.4" : "1.0";
    }
    SECTION("Requested version missing from one source")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::Version, "1.0"sv);
        setup.Packages[1]->Available[0]->Versions.pop_back();
        expected = false;
        secondManifestCalls = 0;
    }
    SECTION("No versions")
    {
        setup.Packages[1]->Available[0]->Versions.clear();
        expected = false;
        secondManifestCalls = 0;
    }
    SECTION("Missing channel")
    {
        setup.Context.Args.AddArg(Execution::Args::Type::Version, "1.0"sv);
        setup.Context.Args.AddArg(Execution::Args::Type::Channel, "Missing"sv);
        expected = false;
        firstManifestCalls = secondManifestCalls = 0;
    }
    SECTION("Unknown search version resolves")
    {
        for (auto& version : setup.Versions) { version->SearchVersion = "Unknown"; }
    }
    SECTION("Latest has no applicable installer")
    {
        setup.Versions[1]->VersionManifest.Installers.clear();
        expected = false;
    }
    SECTION("Only older version matches")
    {
        setup.Versions[1]->VersionManifest.Installers[0].Sha256.back() ^= 1;
        expected = false;
    }
    auto result = setup.Prepare(operation);
    REQUIRE(result.has_value() == expected);
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == firstManifestCalls);
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == secondManifestCalls);
    if (result)
    {
        REQUIRE(result->Manifest.Version == expectedVersion);
        setup.RequireManifestCalls(1);

        std::ostringstream output;
        TestContext normal{ output, std::cin };
        auto normalThreadGlobals = normal.SetForCurrentThread();
        normal.Args = setup.Context.Args;
        normal.Add<Execution::Data::Package>(setup.Packages[0]);
        normal.Add<Execution::Data::InstalledPackageVersion>(std::shared_ptr<IPackageVersion>{});
        if (operation == OperationType::Install && !normal.Args.Contains(Execution::Args::Type::Version))
        {
            normal << SelectLatestApplicableVersion(true);
        }
        else
        {
            normal << GetManifestFromPackage(operation == OperationType::Install) << SelectInstaller;
        }
        REQUIRE_FALSE(normal.IsTerminated());
        REQUIRE(normal.Get<Execution::Data::PackageVersion>() == result->PackageVersion);
        REQUIRE(normal.Get<Execution::Data::Manifest>().Version == result->Manifest.Version);
        REQUIRE(normal.Get<Execution::Data::Installer>()->Url == result->Installer.Url);
    }
}

TEST_CASE("PackageDeduplication_PrepareIgnoresPinsWithoutInstalledVersion", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Pinning;
    auto type = GENERATE(PinType::Pinning, PinType::Blocking, PinType::Gating);
    bool force = GENERATE(false, true);
    bool explicitVersion = GENERATE(false, true);
    TempFile indexFile{ "dedupPinning", ".db" };
    TestHook::SetPinningIndex_Override pinningIndexOverride{ indexFile.GetPath() };
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    if (force) { setup.Context.Args.AddArg(Execution::Args::Type::Force); }
    if (explicitVersion) { setup.Context.Args.AddArg(Execution::Args::Type::Version, "2.4"sv); }
    {
        PinningData pins{ PinningData::Disposition::ReadWrite };
        PinKey key{ setup.Manifests[0].Id, setup.Sources[0]->Details.Identifier };
        if (type == PinType::Pinning) { pins.AddOrUpdatePin(Pin::CreatePinningPin(key)); }
        if (type == PinType::Blocking) { pins.AddOrUpdatePin(Pin::CreateBlockingPin(key)); }
        if (type == PinType::Gating) { pins.AddOrUpdatePin(Pin::CreateGatingPin(key, AppInstaller::Utility::GatedVersion{ "1.*"sv })); }
    }
    auto result = setup.Prepare();
    REQUIRE(result);
    setup.RequireManifestCalls(1);

    setup.Context.Add<Execution::Data::Package>(setup.Packages[0]);
    setup.Context.Add<Execution::Data::InstalledPackageVersion>(std::shared_ptr<IPackageVersion>{});
    if (explicitVersion) { setup.Context << GetManifestFromPackage(true) << SelectInstaller; }
    else { setup.Context << SelectLatestApplicableVersion(true); }
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE(setup.Context.Get<Execution::Data::PackageVersion>() == result->PackageVersion);
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Version == result->Manifest.Version);
}

TEST_CASE("PackageDeduplication_PrepareOnlyParticipatingSources", "[PackageDeduplication][workflow]")
{
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Sources[0]->Details.Priority = -1;
    auto& matches = setup.Context.Get<Execution::Data::SearchResult>().Matches;
    matches.erase(matches.begin());
    std::reverse(matches.begin(), matches.end());
    auto result = setup.Prepare();
    REQUIRE(result);
    REQUIRE(result->Package == setup.Packages[1]);
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == 0);
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringVersionData == 0);
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == 1);
    REQUIRE(setup.Sources[2]->CountOfCallsRequiringManifestData == 1);
}

TEST_CASE("PackageDeduplication_PrepareSnapshot", "[PackageDeduplication][workflow]")
{
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Versions[1]->OnManifest = [&]()
    {
        auto newer = setup.Manifests[0];
        newer.Version = "2.5";
        newer.Installers[0].Url = "https://example.com/new.exe";
        setup.Packages[0]->Available[0]->Versions.insert(setup.Packages[0]->Available[0]->Versions.begin(),
            TestPackageVersion::Make(newer, setup.Sources[0]));
    };
    auto result = setup.Prepare(operation);
    REQUIRE(result);
    REQUIRE(result->Package == setup.Packages[0]);
    REQUIRE(result->PackageVersion == setup.Versions[0]);
    REQUIRE(result->Manifest.Version == "2.4");
    REQUIRE(result->Installer.Url == setup.Manifests[0].Installers[0].Url);
    REQUIRE(setup.Packages[0]->Available[0]->GetLatestVersion()->GetProperty(PackageVersionProperty::Version) == "2.5");
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_PreparePreservesContext", "[PackageDeduplication][workflow]")
{
    bool equivalent = GENERATE(false, true);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Context.Add<Execution::Data::Package>(setup.Packages[1]);
    setup.Context.Add<Execution::Data::PackageVersion>(setup.Versions[1]);
    auto manifest = setup.Manifests[1];
    manifest.Version = "Unrelated";
    setup.Context.Add<Execution::Data::Manifest>(manifest);
    setup.Context.Add<Execution::Data::Installer>(manifest.Installers[0]);
    if (!equivalent) { setup.Versions[1]->VersionManifest.Installers[0].Sha256.back() ^= 1; }
    REQUIRE(PreparePackageDeduplication(setup.Context, OperationType::Install).has_value() == equivalent);
    REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[1]);
    REQUIRE(setup.Context.Get<Execution::Data::PackageVersion>() == setup.Versions[1]);
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Version == "Unrelated");
    REQUIRE(setup.Context.Get<Execution::Data::Installer>()->Url == manifest.Installers[0].Url);
    REQUIRE_FALSE(setup.Context.IsTerminated());
}

TEST_CASE("PackageDeduplication_PrepareCancellation", "[PackageDeduplication][workflow]")
{
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    size_t firstCalls = 0;
    SECTION("Already canceled") { setup.Context.Terminate(E_ABORT); }
    SECTION("Canceled during retrieval")
    {
        setup.Versions[0]->OnManifest = [&]() { setup.Context.Terminate(E_ABORT); };
        firstCalls = 1;
    }
    REQUIRE_FALSE(PreparePackageDeduplication(setup.Context, OperationType::Install));
    REQUIRE(setup.Context.GetTerminationHR() == E_ABORT);
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == firstCalls);
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == 0);
    REQUIRE(setup.Sources[2]->CountOfCallsRequiringManifestData == 0);
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Package));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Manifest));
}

TEST_CASE("PackageDeduplication_PreparePropagatesFailures", "[PackageDeduplication][workflow]")
{
    auto hr = GENERATE(E_ABORT, E_UNEXPECTED, HRESULT_FROM_WIN32(ERROR_TIMEOUT), APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Versions[1]->OnManifest = [hr]() { THROW_HR(hr); };
    REQUIRE_THROWS_HR(PreparePackageDeduplication(setup.Context, OperationType::Install), hr);
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == 1);
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == 1);
    REQUIRE(setup.Sources[2]->CountOfCallsRequiringManifestData == 0);
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Package));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::PackageVersion));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Manifest));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Installer));
}

TEST_CASE("PackageDeduplication_SelectionOptIn", "[PackageDeduplication][workflow]")
{
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    auto behavior = GENERATE(PackageSelectionBehavior::Disabled, PackageSelectionBehavior::Prompt,
        PackageSelectionBehavior::Deduplicate, PackageSelectionBehavior::Prompt | PackageSelectionBehavior::Deduplicate);
    bool enabled = GENERATE(false, true);
    bool promptEnabled = GENERATE(false, true);
    bool policyEnabled = GENERATE(false, true);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::ExperimentalFeatures, policyEnabled ? PolicyState::Enabled : PolicyState::Disabled);
    setup.Settings.Set<Setting::EFPackageDeduplication>(bool{ enabled });
    setup.Settings.Set<Setting::EFInteractivePackageSelection>(bool{ promptEnabled });
    setup.Context.Reporter.SetConsoleStreamsForTest(false);
    setup.Context.Reporter.SetStyle(VisualStyle::NoVT);
    setup.Context << EnsureOneMatchFromSearchResult(operation, behavior);

    bool resolved = enabled && policyEnabled && WI_IsFlagSet(behavior, PackageSelectionBehavior::Deduplicate);
    REQUIRE(setup.Context.IsTerminated() == !resolved);
    REQUIRE(setup.Context.Contains(Execution::Data::PreparedPackageMatch) == resolved);
    REQUIRE(setup.Context.Get<Execution::Data::SearchResult>().Matches.size() == 2);
    REQUIRE(setup.Input.peek() == '2');
    setup.RequireManifestCalls(resolved ? size_t{ 1 } : size_t{ 0 });
    if (resolved)
    {
        REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[0]);
        REQUIRE(setup.Output.str().find(Resource::String::PackageDeduplicationSelected(
            setup.Packages[0]->GetProperty(PackageProperty::Name), setup.Packages[0]->GetProperty(PackageProperty::Id),
            AppInstaller::Utility::LocIndView{ setup.Sources[0]->Details.Name }).get()) != std::string::npos);
    }
    else
    {
        REQUIRE_TERMINATED_WITH(setup.Context, APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND);
    }
}

TEST_CASE("PackageDeduplication_SelectionWithoutPrompt", "[PackageDeduplication][PackageSelection][workflow]")
{
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Settings.Set<Setting::EFInteractivePackageSelection>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(true);
    setup.Context.Reporter.SetStyle(VisualStyle::NoVT);
    SECTION("Console") {}
    SECTION("Redirected") { setup.Context.Reporter.SetConsoleStreamsForTest(false); }
    SECTION("Disabled interactivity") { setup.Context.Args.AddArg(Execution::Args::Type::DisableInteractivity); }
    SECTION("Silent") { setup.Context.Args.AddArg(Execution::Args::Type::Silent); }
    setup.Context << EnsureOneMatchFromSearchResult(OperationType::Install,
        PackageSelectionBehavior::Prompt | PackageSelectionBehavior::Deduplicate);
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[0]);
    REQUIRE(setup.Input.peek() == '2');
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_SelectionFallback", "[PackageDeduplication][PackageSelection][workflow]")
{
    bool canPrompt = GENERATE(false, true);
    bool retrievalFailure = GENERATE(false, true);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Settings.Set<Setting::EFInteractivePackageSelection>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(canPrompt);
    setup.Context.Reporter.SetStyle(VisualStyle::NoVT);
    if (retrievalFailure)
    {
        setup.Versions[1]->OnManifest = []() { THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST); };
    }
    else
    {
        setup.Versions[1]->VersionManifest.Installers[0].Sha256.back() ^= 1;
    }
    setup.Context << EnsureOneMatchFromSearchResult(OperationType::Show,
        PackageSelectionBehavior::Prompt | PackageSelectionBehavior::Deduplicate);
    REQUIRE(setup.Context.IsTerminated() == !canPrompt);
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::PreparedPackageMatch));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Manifest));
    REQUIRE(setup.Context.Get<Execution::Data::SearchResult>().Matches.size() == 2);
    setup.RequireManifestCalls(1);
    if (canPrompt)
    {
        REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[1]);
        REQUIRE(setup.Output.str().find(Resource::String::NumberedSelectionPrompt(2).get()) != std::string::npos);
    }
    else
    {
        REQUIRE_TERMINATED_WITH(setup.Context, APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND);
        REQUIRE(setup.Input.peek() == '2');
    }
    setup.Settings.Set<Setting::EFPackageDeduplication>(false);
    std::ostringstream normalOutput;
    std::istringstream normalInput{ "2\n" };
    TestContext normal{ normalOutput, normalInput };
    auto normalThreadGlobals = normal.SetForCurrentThread();
    normal.Reporter.SetConsoleStreamsForTest(canPrompt);
    normal.Reporter.SetStyle(VisualStyle::NoVT);
    normal.Add<Execution::Data::SearchResult>(setup.Context.Get<Execution::Data::SearchResult>());
    normal << EnsureOneMatchFromSearchResult(OperationType::Show, PackageSelectionBehavior::Prompt | PackageSelectionBehavior::Deduplicate);
    REQUIRE(setup.Output.str() == normalOutput.str());
}

TEST_CASE("PackageDeduplication_SelectionPriority", "[PackageDeduplication][SourcePriority][workflow]")
{
    bool tied = GENERATE(false, true);
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Sources[0]->Details.Priority = 10;
    setup.Sources[1]->Details.Priority = tied ? 10 : 0;
    setup.Sources[2]->Details.Priority = -1;
    std::reverse(setup.Context.Get<Execution::Data::SearchResult>().Matches.begin(), setup.Context.Get<Execution::Data::SearchResult>().Matches.end());
    setup.Context << EnsureOneMatchFromSearchResult(OperationType::Show, PackageSelectionBehavior::Deduplicate);
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[0]);
    REQUIRE(setup.Context.Contains(Execution::Data::PreparedPackageMatch) == tied);
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == (tied ? size_t{ 1 } : size_t{ 0 }));
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == (tied ? size_t{ 1 } : size_t{ 0 }));
    REQUIRE(setup.Sources[2]->CountOfCallsRequiringManifestData == 0);
}

TEST_CASE("PackageDeduplication_SelectionFailures", "[PackageDeduplication][workflow]")
{
    auto hr = GENERATE(E_ABORT, HRESULT_FROM_WIN32(ERROR_CANCELLED), HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED),
        E_UNEXPECTED, E_OUTOFMEMORY, E_INVALIDARG, HRESULT_FROM_WIN32(ERROR_TIMEOUT),
        HRESULT_FROM_WIN32(ERROR_INTERNET_TIMEOUT), HTTP_E_STATUS_NOT_FOUND, APPINSTALLER_CLI_ERROR_INVALID_MANIFEST,
        APPINSTALLER_CLI_ERROR_LIBYAML_ERROR, APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_DATA,
        APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
    bool winrtException = GENERATE(false, true);
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Settings.Set<Setting::EFInteractivePackageSelection>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(false);
    setup.Versions[1]->OnManifest = [=]()
    {
        if (winrtException) { throw winrt::hresult_error(hr); }
        THROW_HR(hr);
    };
    auto select = EnsureOneMatchFromSearchResult(OperationType::Show, PackageSelectionBehavior::Prompt | PackageSelectionBehavior::Deduplicate);
    bool unexpected = hr == E_UNEXPECTED || hr == E_OUTOFMEMORY || hr == E_INVALIDARG;
    bool canceled = hr == E_ABORT || hr == HRESULT_FROM_WIN32(ERROR_CANCELLED) || hr == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED);
    if (unexpected)
    {
        REQUIRE_THROWS(select(setup.Context));
        REQUIRE_FALSE(setup.Context.IsTerminated());
    }
    else
    {
        REQUIRE_NOTHROW(select(setup.Context));
        REQUIRE(setup.Context.GetTerminationHR() == (canceled ? hr : APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND));
    }
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == 1);
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == 1);
    REQUIRE(setup.Sources[2]->CountOfCallsRequiringManifestData == 0);
    REQUIRE(setup.Input.peek() == '2');
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Package));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::PreparedPackageMatch));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Manifest));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Installer));
}

TEST_CASE("PackageDeduplication_CommandSnapshot", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    bool explicitVersion = GENERATE(false, true);
    bool reverse = GENERATE(false, true);
    bool requestedLocale = GENERATE(false, true);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(false);
    setup.Context.Reporter.SetStyle(VisualStyle::NoVT);
    setup.Context.Args.AddArg(Execution::Args::Type::Query, "Editor"sv);
    if (explicitVersion) { setup.Context.Args.AddArg(Execution::Args::Type::Version, "2.4"sv); }
    if (requestedLocale) { setup.Context.Args.AddArg(Execution::Args::Type::Locale, "fr-FR"sv); }
    for (size_t i = 0; i < setup.Manifests.size(); ++i)
    {
        auto& manifest = setup.Manifests[i];
        manifest.DefaultLocalization.Locale = "en-US";
        ManifestLocalization localization;
        localization.Locale = "fr-FR";
        localization.Add<Localization::PackageName>("Localized editor");
        manifest.Localizations.emplace_back(std::move(localization));
        manifest.Installers[0].Locale = "fr-FR";
        setup.Versions[i]->VersionManifest = manifest;
    }
    setup.SetSourceOrder(reverse);
    size_t selected = reverse ? 1 : 0;
    setup.Versions[1]->OnManifest = [&]()
    {
        auto newer = setup.Manifests[selected];
        newer.Version = "2.5";
        newer.DefaultLocalization.Add<Localization::PackageName>("Changed package name");
        newer.Installers[0].Url = "https://example.com/new.exe";
        setup.Packages[selected]->Available[0]->Versions.insert(setup.Packages[selected]->Available[0]->Versions.begin(),
            std::make_shared<TestPackageVersion>(newer, setup.Sources[selected]));
        setup.Versions[selected]->VersionManifest.Installers[0].Url = "https://example.com/changed.exe";
    };
    setup.Context.Override({ "OpenSource", [](TestContext&) {} });
    setup.Context.Override({ SearchSourceForSingle, [](TestContext&) {} });
    if (operation == OperationType::Install)
    {
        setup.Context.Override({ "OpenCompositeSource", [](TestContext&) {} });
        setup.Context.Override({ InstallSinglePackage, [](TestContext&) {}, 1 });
        InstallCommand({}).Execute(setup.Context);
    }
    else if (operation == OperationType::Download)
    {
        setup.Context.Override({ SetDownloadDirectory, [](TestContext&) {} });
        setup.Context.Override({ DownloadInstaller, [](TestContext&) {}, 1 });
        DownloadCommand({}).Execute(setup.Context);
    }
    else
    {
        ShowCommand({}).Execute(setup.Context);
        REQUIRE(setup.Output.str().find(setup.Manifests[selected].Installers[0].Url) != std::string::npos);
    }
    INFO(setup.Output.str());
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[selected]);
    REQUIRE(setup.Context.Get<Execution::Data::PackageVersion>() == setup.Versions[selected]);
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Id == setup.Manifests[selected].Id);
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Version == "2.4");
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().Installers[0].Url == setup.Manifests[selected].Installers[0].Url);
    auto expectedManifest = setup.Manifests[selected];
    expectedManifest.ApplyLocale(requestedLocale || (operation == OperationType::Install && !explicitVersion) ? "fr-FR" : "");
    REQUIRE(setup.Context.Get<Execution::Data::Manifest>().CurrentLocalization.Get<Localization::PackageName>() ==
        expectedManifest.CurrentLocalization.Get<Localization::PackageName>());
    REQUIRE(setup.Context.Get<Execution::Data::Installer>()->Url == setup.Manifests[selected].Installers[0].Url);
    REQUIRE(setup.Output.str().find("Changed package name") == std::string::npos);
    REQUIRE_FALSE(setup.Context.Get<Execution::Data::PreparedPackageMatch>());
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_ParsingAndCancellation", "[PackageDeduplication][workflow]")
{
    PackageDeduplicationTestSetup setup{ 3 };
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Settings.Set<Setting::EFInteractivePackageSelection>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(false);
    bool canceled = true;
    SECTION("Context cancellation")
    {
        setup.Versions[0]->OnManifest = [&]() { setup.Context.Terminate(E_ABORT); };
    }
    SECTION("Task cancellation")
    {
        setup.Versions[0]->OnManifest = []() { throw pplx::task_canceled{}; };
    }
    SECTION("Invalid JSON")
    {
        canceled = false;
        setup.Versions[0]->OnManifest = []() { web::json::value::parse(L"{"); };
    }
    setup.Context << EnsureOneMatchFromSearchResult(OperationType::Show,
        PackageSelectionBehavior::Prompt | PackageSelectionBehavior::Deduplicate);
    REQUIRE(setup.Context.GetTerminationHR() == (canceled ? E_ABORT : APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND));
    REQUIRE(setup.Sources[0]->CountOfCallsRequiringManifestData == 1);
    REQUIRE(setup.Sources[1]->CountOfCallsRequiringManifestData == 0);
    REQUIRE(setup.Sources[2]->CountOfCallsRequiringManifestData == 0);
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::PreparedPackageMatch));
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::Package));
    REQUIRE(setup.Input.peek() == '2');
}

TEST_CASE("PackageDeduplication_PreparedIdentityGuard", "[PackageDeduplication][workflow]")
{
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Context << EnsureOneMatchFromSearchResult(OperationType::Show, PackageSelectionBehavior::Deduplicate);
    REQUIRE_FALSE(setup.Context.IsTerminated());
    SECTION("Wrong operation")
    {
        REQUIRE_THROWS_HR(GetManifestFromPackage(false, OperationType::Download)(setup.Context), E_UNEXPECTED);
    }
    SECTION("Caller not opted in")
    {
        REQUIRE_THROWS_HR(GetManifestFromPackage(false)(setup.Context), E_UNEXPECTED);
    }
    SECTION("Wrong package")
    {
        setup.Context.Add<Execution::Data::Package>(setup.Packages[1]);
        REQUIRE_THROWS_HR(GetManifestFromPackage(false, OperationType::Show)(setup.Context), E_UNEXPECTED);
    }
    SECTION("Wrong version at installer handoff")
    {
        setup.Context << GetManifestFromPackage(false, OperationType::Show);
        setup.Context.Add<Execution::Data::PackageVersion>(setup.Versions[1]);
        REQUIRE_THROWS_HR(SelectInstaller(setup.Context), E_UNEXPECTED);
    }
    SECTION("Operation changed to upgrade")
    {
        setup.Context << GetManifestFromPackage(false, OperationType::Show);
        setup.Context.SetFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);
        REQUIRE_THROWS_HR(SelectInstaller(setup.Context), E_UNEXPECTED);
    }
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_ShowVersionsExcluded", "[PackageDeduplication][workflow]")
{
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(false);
    setup.Context.Args.AddArg(Execution::Args::Type::Query, "Editor"sv);
    setup.Context.Args.AddArg(Execution::Args::Type::ListVersions);
    setup.Context.Override({ "OpenSource", [](TestContext&) {} });
    setup.Context.Override({ SearchSourceForSingle, [](TestContext&) {} });
    ShowCommand({}).Execute(setup.Context);
    REQUIRE_TERMINATED_WITH(setup.Context, APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND);
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::PreparedPackageMatch));
    setup.RequireManifestCalls(0);
}

TEST_CASE("PackageDeduplication_CommandFailureDoesNotChangeSource", "[PackageDeduplication][workflow]")
{
    auto operation = GENERATE(OperationType::Install, OperationType::Download);
    bool hashMismatch = GENERATE(false, true);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Context.Reporter.SetConsoleStreamsForTest(false);
    setup.Context.Args.AddArg(Execution::Args::Type::Query, "Editor"sv);
    setup.Context.Override({ "OpenSource", [](TestContext&) {} });
    setup.Context.Override({ SearchSourceForSingle, [](TestContext&) {} });
    setup.Context.Override({ CheckForExistingInstaller, [](TestContext&) {}, 1 });
    setup.Context.Override({ DownloadInstallerFile, [&](TestContext& context)
    {
        REQUIRE(context.Get<Execution::Data::Package>() == setup.Packages[0]);
        REQUIRE(context.Get<Execution::Data::Installer>()->Url == setup.Manifests[0].Installers[0].Url);
        if (hashMismatch)
        {
            AppInstaller::Utility::DownloadResult result;
            result.Sha256Hash.assign(32, 0x43);
            result.SizeInBytes = 100;
            context.Add<Execution::Data::DownloadHashInfo>({ context.Get<Execution::Data::Installer>()->Sha256, std::move(result) });
        }
        else
        {
            context.Terminate(APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED);
        }
    }, 1 });
    if (operation == OperationType::Install)
    {
        setup.Context.Override({ "OpenCompositeSource", [](TestContext&) {} });
        InstallCommand({}).Execute(setup.Context);
    }
    else
    {
        setup.Context.Override({ SetDownloadDirectory, [](TestContext&) {} });
        DownloadCommand({}).Execute(setup.Context);
    }
    INFO(setup.Output.str());
    REQUIRE(setup.Context.GetTerminationHR() ==
        (hashMismatch ? APPINSTALLER_CLI_ERROR_INSTALLER_HASH_MISMATCH : APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED));
    REQUIRE(setup.Context.Get<Execution::Data::Package>() == setup.Packages[0]);
    REQUIRE(setup.Context.Get<Execution::Data::PackageVersion>() == setup.Versions[0]);
    REQUIRE_FALSE(WI_IsFlagSet(setup.Context.GetFlags(), Execution::ContextFlag::InstallerHashMatched));
    REQUIRE_FALSE(WI_IsFlagSet(setup.Context.GetFlags(), Execution::ContextFlag::InstallerTrusted));
    REQUIRE(setup.Input.peek() == '2');
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_PreservesAgreementsAndTrust", "[PackageDeduplication][workflow]")
{
    using namespace AppInstaller::Manifest;
    auto operation = GENERATE(OperationType::Install, OperationType::Download);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    setup.Sources[1]->Details.TrustLevel = SourceTrustLevel::Trusted;
    bool agreements = false;
    SECTION("Selected package agreements")
    {
        agreements = true;
        setup.Versions[0]->VersionManifest.DefaultLocalization.Add<Localization::Agreements>(
            std::vector<Agreement>{ { "License", "Terms for the selected source", "https://example.com/terms" } });
    }
    SECTION("Selected source untrusted") {}
    SECTION("Selected source trusted") { setup.Sources[0]->Details.TrustLevel = SourceTrustLevel::Trusted; }
    setup.Context <<
        EnsureOneMatchFromSearchResult(operation, PackageSelectionBehavior::Deduplicate) <<
        GetManifestFromPackage(operation == OperationType::Install, operation) <<
        SelectInstaller <<
        ShowPromptsForSinglePackage(true);
    if (agreements)
    {
        REQUIRE_TERMINATED_WITH(setup.Context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
        REQUIRE_FALSE(setup.Context.Args.Contains(Execution::Args::Type::AcceptPackageAgreements));
    }
    else
    {
        REQUIRE_FALSE(setup.Context.IsTerminated());
        auto hash = setup.Context.Get<Execution::Data::Installer>()->Sha256;
        setup.Context.Add<Execution::Data::DownloadHashInfo>({ hash, AppInstaller::Utility::DownloadResult{ hash } });
        setup.Context << VerifyInstallerHash;
        REQUIRE_FALSE(setup.Context.IsTerminated());
        REQUIRE(WI_IsFlagSet(setup.Context.GetFlags(), Execution::ContextFlag::InstallerHashMatched));
        REQUIRE(WI_IsFlagSet(setup.Context.GetFlags(), Execution::ContextFlag::InstallerTrusted) ==
            WI_IsFlagSet(setup.Sources[0]->Details.TrustLevel, SourceTrustLevel::Trusted));
    }
    REQUIRE(setup.Context.Get<Execution::Data::PackageVersion>() == setup.Versions[0]);
    setup.RequireManifestCalls(1);
}

TEST_CASE("PackageDeduplication_LocalManifestExcluded", "[PackageDeduplication][workflow]")
{
    auto operation = GENERATE(OperationType::Install, OperationType::Download, OperationType::Show);
    PackageDeduplicationTestSetup setup;
    auto previousThreadGlobals = setup.Context.SetForCurrentThread();
    setup.Settings.Set<Setting::EFPackageDeduplication>(true);
    setup.Context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
    if (operation == OperationType::Install)
    {
        setup.Context.Override({ InstallSinglePackage, [](TestContext&) {}, 1 });
        InstallCommand({}).Execute(setup.Context);
    }
    else if (operation == OperationType::Download)
    {
        setup.Context.Override({ SetDownloadDirectory, [](TestContext&) {} });
        setup.Context.Override({ DownloadInstaller, [](TestContext&) {}, 1 });
        DownloadCommand({}).Execute(setup.Context);
    }
    else
    {
        ShowCommand({}).Execute(setup.Context);
    }
    REQUIRE_FALSE(setup.Context.IsTerminated());
    REQUIRE_FALSE(setup.Context.Contains(Execution::Data::PreparedPackageMatch));
    setup.RequireManifestCalls(0);
}
