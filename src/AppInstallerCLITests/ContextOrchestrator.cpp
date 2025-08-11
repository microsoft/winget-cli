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

static constexpr DWORD c_DefaultWaitInMs = 5000;

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

TestQueueItem CreateTestItem(std::optional<std::string> packageName = std::nullopt)
{
    TestQueueItem result;

    std::unique_ptr<TestCOMContext> context = std::make_unique<TestCOMContext>();
    // Forcibly initialize the thread globals objects
    context->GetThreadGlobals().SetForCurrentThread();

    TestDataFile testManifest("Manifest-Good.yaml");
    auto manifest = YamlParser::CreateFromPath(testManifest);

    if (packageName)
    {
        manifest.Id = packageName.value();
    }

    context->Add<Data::Manifest>(std::move(manifest));

    result.Context = context.get();

    // Marking it an uninstall removes the extra work adding the items to the installing index
    result.QueueItem = std::make_shared<OrchestratorQueueItem>(OrchestratorQueueItemId(AppInstaller::Utility::ConvertToUTF16(packageName.value_or("package")), L"source"), std::move(context), PackageOperationType::Uninstall);

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

    REQUIRE(operationEvent.wait(c_DefaultWaitInMs));
    REQUIRE(testItem.QueueItem->GetCompletedEvent().wait(c_DefaultWaitInMs));
    REQUIRE(orchestrator.WaitForRunningItems(c_DefaultWaitInMs));
}

TEST_CASE("ContextOrchestrator_Disabled_NewEnqueue", "[context_orchestrator]")
{
    ContextOrchestrator orchestrator;
    auto testItem = CreateTestItem();

    auto reason = AppInstaller::CancelReason::AppShutdown;
    orchestrator.Disable(reason);
    REQUIRE_THROWS_HR(orchestrator.EnqueueAndRunItem(testItem.QueueItem), AppInstaller::ToHRESULT(reason));
}

TEST_CASE("ContextOrchestrator_Disabled_QueueTransition", "[context_orchestrator]")
{
    ContextOrchestrator orchestrator;

    auto testItem = CreateTestItem();

    wil::slim_event_manual_reset downloadEvent;
    testItem.Context->DownloadCallback = [&]() { downloadEvent.wait(); };

    orchestrator.EnqueueAndRunItem(testItem.QueueItem);

    auto reason = AppInstaller::CancelReason::AppShutdown;
    orchestrator.Disable(reason);

    downloadEvent.SetEvent();

    REQUIRE(testItem.QueueItem->GetCompletedEvent().wait(c_DefaultWaitInMs));
    REQUIRE(testItem.Context->IsTerminated());
    // Context translates our shutdown HRs to E_ABORT
    REQUIRE(E_ABORT == testItem.Context->GetTerminationHR());
}

// While item in { Queued, Running } in both queues, cancel everything
TEST_CASE("ContextOrchestrator_CancelAllItems", "[context_orchestrator]")
{
    // Limit to one thread for downloads so we can get a queued item
    ContextOrchestrator orchestrator{ 1 };

    auto downloadQueued = CreateTestItem("downloadQueued");
    auto downloadRunning = CreateTestItem("downloadRunning");
    wil::slim_event_manual_reset downloadBegunEvent;
    wil::slim_event_manual_reset downloadWaitingEvent;
    downloadRunning.Context->DownloadCallback = [&]()
        {
            downloadBegunEvent.SetEvent();
            downloadWaitingEvent.wait();
        };

    auto operationQueued = CreateTestItem("operationQueued");
    auto operationRunning = CreateTestItem("operationRunning");
    wil::slim_event_manual_reset operationBegunEvent;
    wil::slim_event_manual_reset operationWaitingEvent;
    operationRunning.Context->OperationCallback = [&]()
        {
            operationBegunEvent.SetEvent();
            operationWaitingEvent.wait();
        };

    orchestrator.EnqueueAndRunItem(operationRunning.QueueItem);
    orchestrator.EnqueueAndRunItem(operationQueued.QueueItem);
    orchestrator.EnqueueAndRunItem(downloadRunning.QueueItem);
    orchestrator.EnqueueAndRunItem(downloadQueued.QueueItem);

    operationBegunEvent.wait(c_DefaultWaitInMs);
    downloadBegunEvent.wait(c_DefaultWaitInMs);

    INFO("Pre-shutdown state: \n" << orchestrator.GetStatusString());

    auto reason = AppInstaller::CancelReason::AppShutdown;
    orchestrator.Disable(reason);
    orchestrator.CancelQueuedItems(reason);

    operationWaitingEvent.SetEvent();
    downloadWaitingEvent.SetEvent();

    if (!orchestrator.WaitForRunningItems(c_DefaultWaitInMs))
    {
        INFO("Post-wait state: \n" << orchestrator.GetStatusString());
        FAIL("Timed out waiting for orchestrator to empty");
    }

    auto checkQueueItem = [](TestQueueItem& item)
        {
            REQUIRE(item.QueueItem->GetCompletedEvent().wait(0));
            REQUIRE(item.Context->IsTerminated());
            REQUIRE(E_ABORT == item.Context->GetTerminationHR());
        };

    checkQueueItem(downloadQueued);
    checkQueueItem(downloadRunning);
    checkQueueItem(operationQueued);
    checkQueueItem(operationRunning);
}
