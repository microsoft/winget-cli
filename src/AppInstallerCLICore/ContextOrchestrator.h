// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"
#include "ExecutionContextData.h"
#include "CompletionData.h"
#include "COMContext.h"

#include <string_view>

namespace AppInstaller::CLI::Execution
{
    enum OrchestratorQueueItemState
    {
        Queued,
        Running
    };

    struct OrchestratorQueueItem
    {
        OrchestratorQueueItem(OrchestratorQueueItemState state, std::shared_ptr<COMContext> context, HANDLE completedEvent) : m_completedEvent(completedEvent), m_state(state), m_context(context)  {}

        OrchestratorQueueItemState GetState() { return m_state; }
        void SetState(OrchestratorQueueItemState state) { m_state = state; }
        std::shared_ptr<COMContext> GetContext() { return m_context; }
        HANDLE GetCompletedEvent() { return m_completedEvent.get(); }
    private:
        OrchestratorQueueItemState m_state = OrchestratorQueueItemState::Queued;
        std::shared_ptr<COMContext> m_context;
        wil::unique_handle m_completedEvent;
    };

    struct ContextOrchestrator
    {
        static ContextOrchestrator& Instance();

        std::shared_ptr<OrchestratorQueueItem> EnqueueAndRunCommand(std::shared_ptr<COMContext> context);

        void RunContexts();
        std::shared_ptr<OrchestratorQueueItem> GetNextItem();

        void CancelItem(const std::shared_ptr<OrchestratorQueueItem>& item);

        std::shared_ptr<OrchestratorQueueItem> GetQueueItem(std::string id);

    private:
        void EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item);
        void RemoveItemInState(const std::shared_ptr<OrchestratorQueueItem>& item, OrchestratorQueueItemState state);
        static std::string_view GetIdFromItem(const std::shared_ptr<OrchestratorQueueItem>& item);
        std::shared_ptr<OrchestratorQueueItem> FindById(std::string_view packageId);
        void CreateInstallingWriteableSource();

        std::once_flag m_installingWriteableSourceOnceFlag;
        std::shared_ptr<::AppInstaller::Repository::IMutablePackageSource> m_installingWriteableSource = nullptr;
        std::mutex m_queueLock;
        std::deque<std::shared_ptr<OrchestratorQueueItem>> m_queueItems;
    };
}
