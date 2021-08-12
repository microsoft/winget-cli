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
    enum class OrchestratorQueueItemState
    {
        Queued,
        Running
    };

    struct OrchestratorQueueItem
    {
        OrchestratorQueueItem(std::wstring id, std::shared_ptr<COMContext> context) : m_id(id), m_context(std::move(context)) {}

        const OrchestratorQueueItemState GetState() { return m_state; }
        void SetState(OrchestratorQueueItemState state) { m_state = state; }
        const std::shared_ptr<COMContext> GetContext() { return m_context; }
        const wil::unique_event& GetCompletedEvent() { return m_completedEvent; }
        std::wstring_view GetId() { return m_id; }
    private:
        OrchestratorQueueItemState m_state = OrchestratorQueueItemState::Queued;
        std::shared_ptr<COMContext> m_context;
        wil::unique_event m_completedEvent{ wil::EventOptions::ManualReset };
        std::wstring m_id = L"";
    };

    struct ContextOrchestrator
    {
        ContextOrchestrator();
        static ContextOrchestrator& Instance();

        std::shared_ptr<OrchestratorQueueItem> EnqueueAndRunContextOperation(std::wstring identifier, std::shared_ptr<COMContext> context);

        void RunContexts();
        std::shared_ptr<OrchestratorQueueItem> GetNextItem();

        void CancelItem(const std::shared_ptr<OrchestratorQueueItem>& item);

        std::shared_ptr<OrchestratorQueueItem> GetQueueItem(std::wstring id);

    private:
        void EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item);
        void RemoveItemInState(const std::shared_ptr<OrchestratorQueueItem>& item, OrchestratorQueueItemState state);
        static std::string_view GetIdFromItem(const std::shared_ptr<OrchestratorQueueItem>& item);
        std::shared_ptr<OrchestratorQueueItem> FindById(std::wstring_view packageId);

        std::shared_ptr<::AppInstaller::Repository::IMutablePackageSource> m_installingWriteableSource = nullptr;
        std::mutex m_queueLock;
        std::deque<std::shared_ptr<OrchestratorQueueItem>> m_queueItems;
    };
}
