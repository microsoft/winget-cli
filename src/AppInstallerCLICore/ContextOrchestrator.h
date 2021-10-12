// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"
#include "ExecutionContextData.h"
#include "CompletionData.h"
#include "Command.h"
#include "COMContext.h"

#include <string_view>

namespace AppInstaller::CLI::Execution
{
    enum class OrchestratorQueueItemState
    {
        NotQueued,
        Queued,
        Running
    };

    struct OrchestratorQueueItemId
    {
        OrchestratorQueueItemId(std::wstring packageId, std::wstring sourceId) : m_packageId(std::move(packageId)), m_sourceId(std::move(sourceId)) {}
        std::wstring_view GetPackageId() const { return m_packageId; }
        std::wstring_view GetSourceId() const { return m_sourceId; }

        bool IsSame(const OrchestratorQueueItemId& comparisonQueueItemId) const;
    private:
        std::wstring m_packageId;
        std::wstring m_sourceId;
    };

    struct OrchestratorQueueItem
    {
        OrchestratorQueueItem(OrchestratorQueueItemId id, std::unique_ptr<COMContext> context) : m_id(std::move(id)), m_context(std::move(context)) {}

        OrchestratorQueueItemState GetState() const { return m_state; }
        void SetState(OrchestratorQueueItemState state) { m_state = state; }
        COMContext& GetContext() const { return *m_context; }
        const wil::unique_event& GetCompletedEvent() const { return m_completedEvent; }
        const OrchestratorQueueItemId& GetId() const { return m_id; }
        void AddCommand(std::unique_ptr<Command> command) { m_commands.push_back(std::move(command)); }
        const Command& GetNextCommand() const { return *m_commands.front(); }
        std::unique_ptr<Command> PopNextCommand()
        {
            std::unique_ptr<Command> command = std::move(m_commands.front());
            m_commands.pop_front();
            return command;
        }
        bool IsComplete() const { return m_commands.empty(); }

    private:
        OrchestratorQueueItemState m_state = OrchestratorQueueItemState::NotQueued;
        std::unique_ptr<COMContext> m_context;
        wil::unique_event m_completedEvent{ wil::EventOptions::ManualReset };
        OrchestratorQueueItemId m_id;
        std::deque<std::unique_ptr<Command>> m_commands;
    };

    struct OrchestratorQueueItemFactory
    {
        static std::unique_ptr<OrchestratorQueueItem> CreateItemForInstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context);
    };

    struct OrchestratorQueue;

    struct ContextOrchestrator
    {
        ContextOrchestrator();
        static ContextOrchestrator& Instance();

        void EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> queueItem);
        void CancelQueueItem(const OrchestratorQueueItem& item);

        std::shared_ptr<OrchestratorQueueItem> GetQueueItem(const OrchestratorQueueItemId& queueItemId);

        void RequeueItem(std::shared_ptr<OrchestratorQueueItem> queueItem);
        void AddItemManifestToInstallingSource(const OrchestratorQueueItem& queueItem);
        void RemoveItemManifestFromInstallingSource(const OrchestratorQueueItem& queueItem);

    private:
        std::mutex m_queueLock;
        void AddCommandQueue(std::string_view commandName, UINT32 allowedThreads);
        void RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state);

        _Requires_lock_held_(m_queueLock)
        std::shared_ptr<OrchestratorQueueItem> FindById(const OrchestratorQueueItemId& queueItemId);

        std::mutex m_sourceLock;
        std::shared_ptr<::AppInstaller::Repository::IMutablePackageSource> m_installingWriteableSource = nullptr;

        std::map<std::string, std::unique_ptr<OrchestratorQueue>> m_commandQueues;
    };

    // One of the queues used by the orchestrator.
    // All items in the queue execute the same command.
    // The queue allows multiple items to run at the same time, up to a limit.
    struct OrchestratorQueue
    {
        OrchestratorQueue(std::string_view commandName, UINT32 allowedThreads) : m_commandName(commandName), m_allowedThreads(allowedThreads) {}

        // Name of the command this queue can execute
        std::string_view CommandName() const { return m_commandName; }

        // Enqueues an item. If the queue has capacity, immediately starts running it.
        void EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> item, bool isFirstCommand);

        // Removes an item by id, provided that it is in the given state.
        // Returns true if an item was removed.
        // The item can be removed globally from the orchestrator, or from just this queue.
        bool RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state, bool isGlobalRemove);

        // Finds an item by id, if it is in the queue.
        _Requires_lock_held_(m_queueLock)
        std::shared_ptr<OrchestratorQueueItem> FindById(const OrchestratorQueueItemId& queueItemId);

    private:
        // Enqueues an item.
        void EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item, bool isFirstCommand);

        // Runs items while there are more in the queue.
        void RunItems();

        // Gets the next item to run.
        std::shared_ptr<OrchestratorQueueItem> GetNextItem();

        _Requires_lock_held_(m_queueLock)
        std::deque<std::shared_ptr<OrchestratorQueueItem>>::iterator FindIteratorById(const OrchestratorQueueItemId& comparisonQueueItemId);

        std::string_view m_commandName;

        std::mutex m_threadsLock;
        const UINT32 m_allowedThreads;
        UINT32 m_runningThreads = 0;

        std::mutex m_queueLock;
        std::deque<std::shared_ptr<OrchestratorQueueItem>> m_queueItems;
    };
}
