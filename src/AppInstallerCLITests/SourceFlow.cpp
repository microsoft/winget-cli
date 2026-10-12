// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "TestRestRequestHandler.h"
#include <Rest/RestSource.h>
#include <winget/ManifestYamlParser.h>
#include <Commands/DscSourceResource.h>
#include <Commands/InstallCommand.h>
#include <Commands/SourceCommand.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/SourceFlow.h>
#include <winget/Runtime.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;

namespace
{
    using namespace AppInstaller::Manifest;

    struct InstallerEquivalenceTestSetup
    {
        TestUserSettings Settings;
        std::ostringstream Output;
        TestContext Context{ Output, std::cin };
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
}

TEST_CASE("PackageDeduplication_EquivalentInstallers", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_InvalidResolvedVersion", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_IdentityMismatch", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_InvalidHash", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_ExecutionFamily", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_UnsupportedInstaller", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_WholeSetScope", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_InstallerApplicability", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_OnlyPreferredInstaller", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_ApplicableMetadataDifferences", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_DependencySideEffects", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_MatchingDependencies", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_OperationRestrictions", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_UnsupportedArguments", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_PreservesExistingContext", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("PackageDeduplication_ComparisonBounds", "[PackageDeduplication][SourceFlow][workflow]")
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

TEST_CASE("SourcePriority_Arguments", "[SourcePriority][workflow]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::ExperimentalFeatures, GENERATE(PolicyState::NotConfigured, PolicyState::Disabled));
    auto priority = GENERATE("-2147483648"sv, "0"sv, "42"sv, "2147483647"sv);

    Execution::Args addArgs;
    addArgs.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    addArgs.AddArg(Execution::Args::Type::SourceArg, "https://test"sv);
    addArgs.AddArg(Execution::Args::Type::SourcePriority, priority);
    SourceAddCommand sourceAdd({});
    REQUIRE_NOTHROW(sourceAdd.ValidateArguments(addArgs));

    Execution::Args editArgs;
    editArgs.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    editArgs.AddArg(Execution::Args::Type::SourcePriority, priority);
    SourceEditCommand sourceEdit({});
    REQUIRE_NOTHROW(sourceEdit.ValidateArguments(editArgs));

    REQUIRE(Argument::ForType(Execution::Args::Type::SourcePriority).GetVisibility() != Argument::Visibility::Hidden);
}

TEST_CASE("SourcePriority_SearchResult", "[SourcePriority][workflow]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::ExperimentalFeatures, GENERATE(PolicyState::NotConfigured, PolicyState::Disabled));
    auto operationType = GENERATE(OperationType::Install, OperationType::Upgrade, OperationType::Uninstall, OperationType::Repair, OperationType::Export);

    std::vector<int32_t> priorities;
    size_t expectedMatches = 1;

    SECTION("Unique highest priority")
    {
        priorities = { 0, 2, 1 };
    }
    SECTION("Default priorities")
    {
        priorities = { 0, 0, 0 };
        expectedMatches = 3;
    }
    SECTION("Tied highest priority")
    {
        priorities = { 0, 2, 2 };
        expectedMatches = 2;
    }
    SECTION("Negative priorities")
    {
        priorities = { -3, -1, -2 };
    }

    auto manifest = AppInstaller::Manifest::YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    std::vector<AppInstaller::Manifest::Manifest> versions{ manifest };
    std::vector<std::shared_ptr<TestSource>> sources;
    SearchResult searchResult;

    for (int32_t priority : priorities)
    {
        auto source = std::make_shared<TestSource>();
        source->Details.Priority = priority;
        auto package = TestCompositePackage::Make(versions, source);
        if (operationType != OperationType::Install)
        {
            package->Installed = TestPackage::Make(manifest, TestPackage::MetadataMap{}, source);
        }

        searchResult.Matches.emplace_back(package, PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });
        sources.emplace_back(std::move(source));
    }

    auto expectedPackage = searchResult.Matches[1].Package;
    if (operationType != OperationType::Install)
    {
        expectedMatches = priorities.size();
    }

    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Add<Execution::Data::SearchResult>(std::move(searchResult));
    context << EnsureOneMatchFromSearchResult(operationType);

    INFO(output.str());
    REQUIRE(context.Get<Execution::Data::SearchResult>().Matches.size() == expectedMatches);
    REQUIRE(context.GetTerminationHR() == (expectedMatches == 1 ? S_OK : APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND));
    REQUIRE((output.str().find(Resource::LocString(Resource::String::MultiplePackagesFoundFilteredBySourcePriority).get()) != std::string::npos) ==
        (expectedMatches < priorities.size()));

    if (expectedMatches == 1)
    {
        REQUIRE(context.Get<Execution::Data::Package>() == expectedPackage);
    }
}

TEST_CASE("Search_ManifestResolution_BeforeSourcePriority", "[RestSource][SourcePriority][workflow]")
{
    namespace RepositoryRest = AppInstaller::Repository::Rest;
    bool restMatches = GENERATE(false, true);
    CAPTURE(restMatches);
    auto searchResponse = web::json::value::parse(LR"({
        "Data": [{
            "PackageIdentifier": "Foo.Bar", "PackageName": "Unrelated application", "Publisher": "Foo",
            "Versions": [{ "PackageVersion": "Unknown" }]
        }]
    })");
    auto manifestResponse = web::json::value::parse(LR"({
        "Data": {
            "PackageIdentifier": "Foo.Bar",
            "Versions": [{
                "PackageVersion": "1.0.0",
                "DefaultLocale": {
                    "PackageLocale": "en-US", "PackageName": "Bar", "Publisher": "Foo",
                    "License": "MIT", "ShortDescription": "Example application"
                },
                "Installers": [{
                    "Architecture": "x64", "InstallerType": "exe", "InstallerUrl": "https://example.com/installer.exe",
                    "InstallerSha256": "011048877dfaef109801b3f3ab2b60afc74f3fc4f7b3430e0c897f5da1df84b6"
                }]
            }]
        }
    })");
    if (restMatches)
    {
        manifestResponse[L"Data"][L"Versions"][0][L"DefaultLocale"][L"Moniker"] = web::json::value::string(L"tool");
    }
    size_t searches = 0;
    size_t lookups = 0;
    auto handler = std::make_shared<TestRestRequestHandler>(
        [&](web::http::http_request request) -> pplx::task<web::http::http_response>
        {
            web::http::http_response response{ web::http::status_codes::BadRequest };
            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.headers().set_cache_control(L"no-store");
            if (request.method() == web::http::methods::POST)
            {
                ++searches;
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(searchResponse);
            }
            else if (request.method() == web::http::methods::GET)
            {
                ++lookups;
                response.set_status_code(web::http::status_codes::OK);
                response.set_body(manifestResponse);
            }
            return pplx::task_from_result(response);
        });
    AppInstaller::Http::HttpClientHelper helper{ handler };
    SourceDetails details;
    details.Identifier = "RestSource";
    details.Priority = 10;
    auto rest = std::make_shared<RepositoryRest::RestSource>(details, SourceInformation{},
        RepositoryRest::RestClient::Create("https://restsource.com/api", {}, {}, helper,
            RepositoryRest::Schema::IRestClient::Information{ "RestSource", { "1.4.0" } }));

    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Query, "tool"sv);
    context << GetSearchRequestForSingle;
    auto results = rest->Search(context.Get<Execution::Data::SearchRequest>());
    REQUIRE(results.Matches.size() == (restMatches ? size_t{ 1 } : size_t{ 0 }));

    AppInstaller::Manifest::Manifest manifest;
    manifest.Id = "Example.Tool";
    manifest.Version = "1.0.0";
    manifest.Moniker = "tool";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>("Example Tool");
    auto otherSource = std::make_shared<TestSource>();
    otherSource->Details.Priority = 0;
    auto otherPackage = TestCompositePackage::Make(std::vector<AppInstaller::Manifest::Manifest>{ manifest }, otherSource);
    auto expectedPackage = restMatches ? results.Matches[0].Package : otherPackage;
    results.Matches.emplace_back(otherPackage, PackageMatchFilter{ PackageMatchField::Moniker, MatchType::Exact, "tool" });
    context.Add<Execution::Data::SearchResult>(std::move(results));
    context << EnsureOneMatchFromSearchResult(OperationType::Install);

    INFO(output.str());
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(context.Get<Execution::Data::Package>() == expectedPackage);
    REQUIRE(searches == 1);
    REQUIRE(lookups == 1);
}

TEST_CASE("SourcePriority_SourceOutput", "[SourcePriority][workflow]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::ExperimentalFeatures, GENERATE(PolicyState::NotConfigured, PolicyState::Disabled));

    SourceDetails source;
    source.Name = "PriorityTestSource";
    source.Priority = GENERATE(-1, 0, 42);

    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Add<Execution::Data::SourceList>(std::vector<SourceDetails>{ source });

    SECTION("List")
    {
        context.Args.AddArg(Execution::Args::Type::SourceName, source.Name);
        context << ListSources;
        REQUIRE(output.str().find(Resource::LocString(Resource::String::SourceListPriority).get()) != std::string::npos);
        REQUIRE(output.str().find(std::to_string(source.Priority)) != std::string::npos);
    }
    SECTION("Export")
    {
        context << ExportSourceList;
        auto json = ConvertToJson(output.str());
        REQUIRE(json["Priority"].isInt());
        REQUIRE(json["Priority"].asInt() == source.Priority);
    }

    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("SourcePriority_DscSource", "[SourcePriority][workflow]")
{
    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::ExperimentalFeatures, GENERATE(PolicyState::NotConfigured, PolicyState::Disabled));
    SetSetting(Stream::UserSources, "Sources:"sv);
    RemoveSetting(Stream::SourcesMetadata);
    TestHook_ClearSourceFactoryOverrides();

    TestSourceFactory factory{ [](const SourceDetails& details) { return std::make_shared<TestSource>(details); } };
    auto clearFactoryOverrides = wil::scope_exit([]() { TestHook_ClearSourceFactoryOverrides(); });
    TestHook_SetSourceFactoryOverride("Microsoft.Test", factory);

    Json::Value input{ Json::ValueType::objectValue };
    input["name"] = "PriorityTestSource";
    input["argument"] = "priority-test";
    input["type"] = "Microsoft.Test";
    input["priority"] = GENERATE(-1, 0, 42);

    auto invoke = [&](Execution::Args::Type function, bool modifiesSource = false)
    {
        std::istringstream inputStream{ Json::writeString(Json::StreamWriterBuilder{}, input) };
        std::ostringstream output;
        TestContext context{ output, inputStream };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(function);
        if (modifiesSource)
        {
            context.Override({ EnsureRunningAsAdmin, [](TestContext& subContext)
            {
                subContext.Reporter.SetChannel(Execution::Reporter::Channel::Json);
            } });
        }

        DscSourceResource command({});
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

    auto result = invoke(Execution::Args::Type::DscResourceFunctionSet, true);
    REQUIRE(result[0]["priority"] == input["priority"]);

    result = invoke(Execution::Args::Type::DscResourceFunctionGet);
    REQUIRE(result[0]["priority"] == input["priority"]);

    result = invoke(Execution::Args::Type::DscResourceFunctionTest);
    REQUIRE(result[0]["_inDesiredState"].asBool());

    input["priority"] = input["priority"].asInt() + 1;
    result = invoke(Execution::Args::Type::DscResourceFunctionTest);
    REQUIRE_FALSE(result[0]["_inDesiredState"].asBool());

    result = invoke(Execution::Args::Type::DscResourceFunctionSet, true);
    REQUIRE(result.size() == 2);
    REQUIRE(result[0]["priority"] == input["priority"]);
    REQUIRE(result[1].size() == 1);
    REQUIRE(result[1][0].asString() == "priority");

    result = invoke(Execution::Args::Type::DscResourceFunctionSet);
    REQUIRE(result.size() == 2);
    REQUIRE(result[1].empty());

    result = invoke(Execution::Args::Type::DscResourceFunctionExport);
    auto source = std::find_if(result.begin(), result.end(), [&](const Json::Value& value) { return value["name"] == input["name"]; });
    REQUIRE(source != result.end());
    REQUIRE((*source)["priority"] == input["priority"]);
}

void OverrideForSourceAddWithAgreements(TestContext& context, bool isAddExpected = true)
{
    context.Override({ EnsureRunningAsAdmin, [](TestContext&)
    {
    } });

    if (isAddExpected)
    {
        context.Override({ AddSource, [](TestContext&)
        {
        } });
    }

    context.Override({ CreateSourceForSourceAdd, [](TestContext& context)
    {
        auto testSource = std::make_shared<TestSource>();
        testSource->Information.SourceAgreementsIdentifier = "AgreementsIdentifier";
        testSource->Information.SourceAgreements.emplace_back("Agreement Label", "Agreement Text", "https://test");
        testSource->Information.RequiredPackageMatchFields.emplace_back("Market");
        testSource->Information.RequiredQueryParameters.emplace_back("Market");
        context << Workflow::HandleSourceAgreements(Source{ testSource });
    } });
}

TEST_CASE("SourceAddFlow_Agreement", "[SourceAddFlow][workflow]")
{
    std::ostringstream sourceAddOutput;
    TestContext context{ sourceAddOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForSourceAddWithAgreements(context);
    context.Args.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    context.Args.AddArg(Execution::Args::Type::SourceType, "Microsoft.Test"sv);
    context.Args.AddArg(Execution::Args::Type::SourceArg, "TestArg"sv);
    context.Args.AddArg(Execution::Args::Type::AcceptSourceAgreements);

    SourceAddCommand sourceAdd({});
    sourceAdd.Execute(context);
    INFO(sourceAddOutput.str());

    // Verify agreements are shown
    REQUIRE(sourceAddOutput.str().find("Agreement Label") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("Agreement Text") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("https://test") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find(Resource::LocString(Resource::String::SourceAgreementsMarketMessage).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("SourceAddFlow_Agreement_Prompt_Yes", "[SourceAddFlow][workflow]")
{
    // Accept the agreements by saying "Yes" at the prompt
    std::istringstream sourceAddInput{ "y" };
    std::ostringstream sourceAddOutput;
    TestContext context{ sourceAddOutput, sourceAddInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForSourceAddWithAgreements(context);
    context.Args.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    context.Args.AddArg(Execution::Args::Type::SourceType, "Microsoft.Test"sv);
    context.Args.AddArg(Execution::Args::Type::SourceArg, "TestArg"sv);

    SourceAddCommand sourceAdd({});
    sourceAdd.Execute(context);
    INFO(sourceAddOutput.str());

    // Verify agreements are shown
    REQUIRE(sourceAddOutput.str().find("Agreement Label") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("Agreement Text") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("https://test") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find(Resource::LocString(Resource::String::SourceAgreementsMarketMessage).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("SourceAddFlow_Agreement_Prompt_No", "[SourceAddFlow][workflow]")
{
    // Accept the agreements by saying "No" at the prompt
    std::istringstream sourceAddInput{ "n" };
    std::ostringstream sourceAddOutput;
    TestContext context{ sourceAddOutput, sourceAddInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForSourceAddWithAgreements(context, false);
    context.Args.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    context.Args.AddArg(Execution::Args::Type::SourceType, "Microsoft.Test"sv);
    context.Args.AddArg(Execution::Args::Type::SourceArg, "TestArg"sv);

    SourceAddCommand sourceAdd({});
    sourceAdd.Execute(context);
    INFO(sourceAddOutput.str());

    // Verify agreements are shown
    REQUIRE(sourceAddOutput.str().find("Agreement Label") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("Agreement Text") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("https://test") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find(Resource::LocString(Resource::String::SourceAgreementsMarketMessage).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED);
}

TEST_CASE("OpenSource_WithCustomHeader", "[OpenSource][CustomHeader]")
{
    SetSetting(Stream::UserSources, R"(Sources:)"sv);
    TestHook_ClearSourceFactoryOverrides();

    SourceDetails details;
    details.Name = "restsource";
    details.Type = "Microsoft.Rest";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    std::string customHeader = "Test custom header in Open source Flow";

    bool receivedCustomHeader = false;
    TestSourceFactory factory{
        [&](const SourceDetails& sd, std::optional<std::string> header)
        {
            receivedCustomHeader = header.value() == customHeader;
            return std::shared_ptr<ISource>(new TestSource(sd));
        } };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    TestProgress progress;
    AddSource(details, progress);

    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Query, "TestQuery"sv);
    context.Args.AddArg(Execution::Args::Type::CustomHeader, customHeader);
    context.Args.AddArg(Execution::Args::Type::Source, details.Name);

    AppInstaller::CLI::Workflow::OpenSource()(context);
    REQUIRE(receivedCustomHeader);
}

TEST_CASE("SourceResetFlow_ByNameResetsTombstonedDefaultSource", "[SourceResetFlow][workflow]")
{
    SetSetting(Stream::UserSources, R"(
Sources:
  - Name: winget
    Type: ""
    Arg: ""
    Data: ""
    IsTombstone: true
)"sv);

    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
    context.Args.AddArg(Execution::Args::Type::SourceName, "winget"sv);

    SourceResetCommand sourceReset({});
    sourceReset.Execute(context);

    INFO(output.str());
    REQUIRE(context.GetTerminationHR() == S_OK);

    auto sources = Source::GetCurrentSources();
    auto winget = std::find_if(
        sources.begin(),
        sources.end(),
        [](const SourceDetails& sd) { return sd.Name == "winget"; });
    REQUIRE(winget != sources.end());
    REQUIRE(winget->Origin == SourceOrigin::Default);
}
