// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include <Commands/SourceCommand.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/SourceFlow.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;

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
