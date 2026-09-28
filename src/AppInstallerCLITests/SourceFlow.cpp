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
#include <Commands/SourceCommand.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/SourceFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;

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

TEST_CASE("PackageSelection_SearchResult", "[PackageSelection][SourcePriority][workflow]")
{
    TestUserSettings settings;
    auto width = GENERATE(size_t{20}, size_t{120});
    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{width} };
    auto operation = GENERATE(OperationType::Install, OperationType::Show, OperationType::Download);
    auto manifest = AppInstaller::Manifest::YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    std::vector<AppInstaller::Manifest::Manifest> versions{ manifest };
    auto firstSource = std::make_shared<TestSource>();
    auto secondSource = std::make_shared<TestSource>();
    auto lowPriority = std::make_shared<TestSource>();
    firstSource->Details.Name = "FirstSource";
    secondSource->Details.Name = "SecondSource";
    SearchResult result;
    result.Matches.emplace_back(TestCompositePackage::Make(versions, firstSource),
        PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });
    result.Matches.emplace_back(TestCompositePackage::Make(versions, secondSource),
        PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });
    auto expectedPackage = result.Matches[1].Package;
    bool expectPrompt = true;
    bool expectSource = true;
    size_t expectedRows = 2;

    SECTION("Same identity across sources")
    {
    }
    SECTION("Single source")
    {
        result.Matches[1].Package = TestCompositePackage::Make(versions, firstSource);
        expectedPackage = result.Matches[1].Package;
        expectSource = false;
    }
    SECTION("Multiple available sources for a candidate")
    {
        auto package = TestCompositePackage::Make(versions, firstSource);
        package->Available.emplace_back(TestPackage::Make(versions, secondSource));
        result.Matches[0].Package = package;
        expectedRows = 3;
    }
    SECTION("Unique source priority")
    {
        secondSource->Details.Priority = 1;
        expectPrompt = false;
    }
    SECTION("Priority tie")
    {
        firstSource->Details.Priority = 1;
        secondSource->Details.Priority = 1;
        auto excluded = TestCompositePackage::Make(versions, lowPriority);
        result.Matches.insert(result.Matches.begin(), ResultMatch{ excluded,
            PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id } });
    }
    SECTION("Single match")
    {
        result.Matches.erase(result.Matches.begin());
        expectPrompt = false;
    }

    std::istringstream input{ "2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Reporter.SetStyle(VisualStyle::NoVT);
    context.Add<Execution::Data::SearchResult>(std::move(result));
    context << EnsureOneMatchFromSearchResult(operation, true);

    INFO(output.str());
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.Get<Execution::Data::Package>() == expectedPackage);
    REQUIRE((output.str().find(Resource::String::PackageSelectionPrompt(2).get()) != std::string::npos) == expectPrompt);
    if (expectPrompt)
    {
        auto tableStart = output.str().find("\n# ");
        REQUIRE(tableStart != std::string::npos);
        auto tableEnd = output.str().find("\n\n", tableStart + 1);
        REQUIRE(tableEnd != std::string::npos);
        auto tableText = output.str().substr(tableStart + 1, tableEnd - tableStart - 1);
        REQUIRE(tableText.find("\n1 ") != std::string::npos);
        REQUIRE(tableText.find("\n2 ") != std::string::npos);
        std::istringstream tableStream{ tableText };
        std::string line;
        size_t lineCount = 0;
        while (std::getline(tableStream, line))
        {
            REQUIRE(AppInstaller::Utility::UTF8ColumnWidth(line) < width);
            ++lineCount;
        }
        REQUIRE(lineCount == expectedRows + 2);
        if (width == 120)
        {
            REQUIRE(tableText.find(manifest.DefaultLocalization.Get<AppInstaller::Manifest::Localization::PackageName>()) != std::string::npos);
            REQUIRE(tableText.find(manifest.Id) != std::string::npos);
            REQUIRE(tableText.find(manifest.Version) != std::string::npos);
            REQUIRE((tableText.find(Resource::LocString{ Resource::String::SearchSource }.get()) != std::string::npos) == expectSource);
            REQUIRE((tableText.find("FirstSource") != std::string::npos) == expectSource);
            REQUIRE((tableText.find("SecondSource") != std::string::npos) == expectSource);
        }
        else
        {
            REQUIRE(tableText.find("\xE2\x80\xA6") != std::string::npos);
        }
    }
    else
    {
        REQUIRE(input.peek() == '2');
    }
    REQUIRE(firstSource->CountOfCallsRequiringManifestData == 0);
    REQUIRE(secondSource->CountOfCallsRequiringManifestData == 0);
}

TEST_CASE("PackageSelection_SourceRowIdentity", "[PackageSelection][workflow]")
{
    struct PrimaryPackage : TestCompositePackage
    {
        using TestCompositePackage::TestCompositePackage;

        LocIndString GetProperty(PackageProperty property) const override
        {
            return Available.at(1)->GetProperty(property);
        }
    };

    TestUserSettings settings;
    TestHook::SetConsoleWidth_Override widthOverride{ std::optional<size_t>{120} };
    auto operation = GENERATE(OperationType::Install, OperationType::Show, OperationType::Download);
    bool differentName = GENERATE(false, true);
    bool differentId = GENERATE(false, true);
    auto manifest = AppInstaller::Manifest::YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
    manifest.Id = "Public.App";
    manifest.Version = "1.0";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>("PublicName");
    auto firstSource = std::make_shared<TestSource>();
    auto secondSource = std::make_shared<TestSource>();
    firstSource->Details.Name = "FirstSource";
    secondSource->Details.Name = "SecondSource";
    auto package = std::make_shared<PrimaryPackage>(std::vector{ manifest }, firstSource);

    std::string secondName = differentName ? "PrivateName" : "PublicName";
    std::string secondId = differentId ? "Private.App" : "Public.App";
    manifest.Id = secondId;
    manifest.Version = "2.0";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>(secondName);
    package->Available.emplace_back(TestPackage::Make(std::vector{ manifest }, secondSource));
    manifest.Version = "3.0";
    package->Available.emplace_back(TestPackage::Make(std::vector{ manifest }, secondSource));

    SearchResult result;
    result.Matches.emplace_back(package, PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, secondId });
    manifest.Id = "Other.App";
    manifest.Version = "4.0";
    manifest.DefaultLocalization.Add<AppInstaller::Manifest::Localization::PackageName>("OtherName");
    result.Matches.emplace_back(TestCompositePackage::Make(std::vector{ manifest }, firstSource),
        PackageMatchFilter{ PackageMatchField::Id, MatchType::Exact, manifest.Id });

    std::istringstream input{ "1\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    context.Reporter.SetStyle(VisualStyle::NoVT);
    context.Add<Execution::Data::SearchResult>(std::move(result));
    context << EnsureOneMatchFromSearchResult(operation, true);

    INFO(output.str());
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.Get<Execution::Data::Package>() == package);
    auto tableStart = output.str().find("\n# ");
    REQUIRE(tableStart != std::string::npos);
    auto tableEnd = output.str().find("\n\n", tableStart + 1);
    REQUIRE(tableEnd != std::string::npos);
    std::istringstream tableStream{ output.str().substr(tableStart + 1, tableEnd - tableStart - 1) };
    std::string line;
    REQUIRE(static_cast<bool>(std::getline(tableStream, line)));
    REQUIRE(static_cast<bool>(std::getline(tableStream, line)));
    std::vector<std::vector<std::string>> expectedRows{
        { "1", "PublicName", "Public.App", "1.0", "FirstSource" },
        differentName || differentId ? std::vector<std::string>{ secondName, secondId, "2.0", "SecondSource" } :
            std::vector<std::string>{ "2.0", "SecondSource" },
        { "3.0", "SecondSource" },
        { "2", "OtherName", "Other.App", "4.0", "FirstSource" }
    };
    for (const auto& expectedRow : expectedRows)
    {
        REQUIRE(static_cast<bool>(std::getline(tableStream, line)));
        std::istringstream rowStream{ line };
        std::vector<std::string> fields;
        std::string field;
        while (rowStream >> field)
        {
            fields.emplace_back(std::move(field));
        }
        REQUIRE(fields == expectedRow);
    }
    REQUIRE_FALSE(std::getline(tableStream, line));
    REQUIRE(firstSource->CountOfCallsRequiringManifestData == 0);
    REQUIRE(secondSource->CountOfCallsRequiringManifestData == 0);
}

TEST_CASE("PackageSelection_Unavailable", "[PackageSelection][workflow]")
{
    TestUserSettings settings;
    std::istringstream input{ "2\n" };
    std::ostringstream output;
    TestContext context{ output, input };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Reporter.SetConsoleStreamsForTest(true);
    auto source = CreateTestSource({ TSR::TestQuery_ReturnTwo });
    auto result = source->Search({});
    auto operation = OperationType::Install;
    bool allowSelection = true;
    HRESULT expectedError = APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND;

    SECTION("Default workflow")
    {
        allowSelection = false;
    }
    SECTION("Context disabled")
    {
        context.SetFlags(Execution::ContextFlag::DisableInteractivity);
    }
    SECTION("Argument disabled")
    {
        context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    }
    SECTION("Setting disabled")
    {
        settings.Set<Setting::InteractivityDisable>(true);
    }
    SECTION("Silent")
    {
        context.Args.AddArg(Execution::Args::Type::Silent);
    }
    SECTION("Redirected streams")
    {
        context.Reporter.SetConsoleStreamsForTest(false);
    }
    SECTION("Hidden output")
    {
        context.Reporter.SetChannel(Execution::Reporter::Channel::Disabled);
    }
    SECTION("Truncated results")
    {
        result.Truncated = true;
    }
    SECTION("No matches")
    {
        result.Matches.clear();
        expectedError = APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND;
    }
    SECTION("Excluded operation")
    {
        operation = GENERATE(OperationType::Upgrade, OperationType::Uninstall, OperationType::Repair,
            OperationType::Export, OperationType::Pin, OperationType::Search, OperationType::List, OperationType::Completion);
    }

    context.Add<Execution::Data::SearchResult>(std::move(result));
    context << EnsureOneMatchFromSearchResult(operation, allowSelection);
    INFO(output.str());
    REQUIRE_TERMINATED_WITH(context, expectedError);
    REQUIRE_FALSE(context.Contains(Execution::Data::Package));
    REQUIRE(input.peek() == '2');
    REQUIRE(output.str().find(Resource::String::PackageSelectionPrompt(2).get()) == std::string::npos);
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
