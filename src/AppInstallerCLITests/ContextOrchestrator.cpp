// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <ContextOrchestrator.h>
#include <winget/ManifestYamlParser.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;

struct TestCOMContext : public COMContext
{
    TestCOMContext() = default;

    std::function<void()> DownloadCallback;

    void InvokeDownload()
    {
        if (DownloadCallback)
        {
            DownloadCallback();
        }
    }

    std::function<void()> OperationCallback;

    void InvokeOperation()
    {
        if (OperationCallback)
        {
            OperationCallback();
        }
    }
};

struct TestDownloadCommand : public Command
{
    TestDownloadCommand() : Command("download", "tests") {}

    Resource::LocString ShortDescription() const override { return {}; }
    Resource::LocString LongDescription() const override { return {}; }
    void ValidateArguments(Execution::Args&) const override {}

    void Execute(Context& context) const override
    {
        static_cast<TestCOMContext*>(&context)->InvokeDownload();
    }
};

struct TestOperationCommand : public Command
{
    TestOperationCommand() : Command("operation", "tests") {}

    Resource::LocString ShortDescription() const override { return {}; }
    Resource::LocString LongDescription() const override { return {}; }
    void ValidateArguments(Execution::Args&) const override {}

    void Execute(Context& context) const override
    {
        static_cast<TestCOMContext*>(&context)->InvokeOperation();
    }
};

struct TestQueueItem
{
    TestCOMContext* Context = nullptr;
    std::shared_ptr<OrchestratorQueueItem> QueueItem;
};

TestQueueItem CreateTestItem()
{
    TestQueueItem result;

    std::unique_ptr<TestCOMContext> context = std::make_unique<TestCOMContext>();
    // Forcibly initialize the thread globals objects
    context->GetThreadGlobals().SetForCurrentThread();

    TestDataFile testManifest("Manifest-Good.yaml");
    context->Add<Data::Manifest>(YamlParser::CreateFromPath(testManifest));

    result.Context = context.get();

    result.QueueItem = std::make_shared<OrchestratorQueueItem>(OrchestratorQueueItemId(L"package", L"source"), std::move(context), PackageOperationType::Install);

    result.QueueItem->AddCommand(std::make_unique<TestDownloadCommand>());
    result.QueueItem->AddCommand(std::make_unique<TestOperationCommand>());

    return result;
}

// Runs an item through the orchestrator to ensure the basic functionality
TEST_CASE("ContextOrchestrator_UnitTestExecution", "[context_orchestrator]")
{
    ContextOrchestrator orchestrator;

    auto testItem = CreateTestItem();

    wil::slim_event_manual_reset operationEvent;
    testItem.Context->OperationCallback = [&]() { operationEvent.SetEvent(); };

    orchestrator.EnqueueAndRunItem(testItem.QueueItem);

    REQUIRE(operationEvent.wait(5000));
    REQUIRE(orchestrator.WaitForRunningItems(5000));
}

TEST_CASE("ContextOrchestrator_Disabled_NewEnqueue", "[context_orchestrator]")
{
}

TEST_CASE("ContextOrchestrator_Disabled_QueueTransition", "[context_orchestrator]")
{
}

// While item in { Queued, Running, Cancelled } in either queue
// Doubles as the "wait for empty queues" test [update to take wait timeout]
TEST_CASE("ContextOrchestrator_CancelAllItems", "[context_orchestrator]")
{
}
