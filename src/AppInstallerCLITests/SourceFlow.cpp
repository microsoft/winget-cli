// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
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
