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

    struct OrchestratorQueueItemId
    {
        OrchestratorQueueItemId(std::wstring packageId, std::wstring sourceId) : m_packageId(std::move(packageId)), m_sourceId(std::move(sourceId)) {}
        std::wstring_view GetPackageId() const { return m_packageId; }
        std::wstring_view GetSourceId() const { return m_sourceId; }

        bool IsSame(const OrchestratorQueueItemId*);
    private:
        std::wstring m_packageId;
        std::wstring m_sourceId;
    };

    struct OrchestratorQueueItem
    {
        OrchestratorQueueItem(std::shared_ptr<OrchestratorQueueItemId> id, std::shared_ptr<COMContext> context) : m_id(std::move(id)), m_context(std::move(context)) {}

        OrchestratorQueueItemState GetState() const { return m_state; }
        void SetState(OrchestratorQueueItemState state) { m_state = state; }
        std::shared_ptr<COMContext> GetContext() const { return m_context; }
        wil::unique_event& GetCompletedEvent() { return m_completedEvent; }
        std::shared_ptr<OrchestratorQueueItemId> GetId() const { return m_id; }
    private:
        OrchestratorQueueItemState m_state = OrchestratorQueueItemState::Queued;
        std::shared_ptr<COMContext> m_context;
        wil::unique_event m_completedEvent{ wil::EventOptions::ManualReset };
        std::shared_ptr<OrchestratorQueueItemId> m_id;
    };

    struct OrchestratorQueueItemFactory
    {
        static std::shared_ptr<OrchestratorQueueItem> CreateItemForInstall(std::wstring packageId, std::wstring sourceId, std::shared_ptr<COMContext> context);
    };

    struct ContextOrchestrator
    {
        ContextOrchestrator();
        static ContextOrchestrator& Instance();

        void EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> queueItem);
        void CancelQueueItem(const std::shared_ptr<OrchestratorQueueItem>& item);

        std::shared_ptr<OrchestratorQueueItem> GetQueueItem(const OrchestratorQueueItemId* queueItemId);

    private:
        std::mutex m_queueLock;
        void RunItems();
        std::shared_ptr<OrchestratorQueueItem> GetNextItem();
        void EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item);
        void RemoveItemInState(const std::shared_ptr<OrchestratorQueueItem>& item, OrchestratorQueueItemState state);

        _Requires_lock_held_(m_queueLock)
        std::shared_ptr<OrchestratorQueueItem> FindById(const OrchestratorQueueItemId* queueItemId);

        std::shared_ptr<::AppInstaller::Repository::IMutablePackageSource> m_installingWriteableSource = nullptr;
        std::deque<std::shared_ptr<OrchestratorQueueItem>> m_queueItems;
    };
}
