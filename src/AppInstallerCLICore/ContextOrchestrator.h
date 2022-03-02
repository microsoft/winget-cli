// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <winget/RepositorySource.h>
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
        // Created but not yet queued
        NotQueued,
        // Queued and waiting to be run
        Queued,
        // Running in the thread pool
        Running,
        // Cancelled before it was run; will be deleted when we try to run it
        Cancelled
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

    struct OrchestratorQueue;

    enum class PackageOperationType
    {
        None,
        Install,
        Upgrade,
        Uninstall,
    };

    struct OrchestratorQueueItem
    {
        OrchestratorQueueItem(OrchestratorQueueItemId id, std::unique_ptr<COMContext> context, PackageOperationType operationType) :
            m_id(std::move(id)), m_context(std::move(context)), m_operationType(operationType) {}

        OrchestratorQueueItemState GetState() const { return m_state; }
        void SetState(OrchestratorQueueItemState state) { m_state = state; }

        OrchestratorQueue* GetCurrentQueue() const { return m_currentQueue; }
        void SetCurrentQueue(OrchestratorQueue* currentQueue) { m_currentQueue = currentQueue; }

        COMContext& GetContext() const { return *m_context; }
        const wil::unique_event& GetCompletedEvent() const { return m_completedEvent; }
        const OrchestratorQueueItemId& GetId() const { return m_id; }

        void AddCommand(std::unique_ptr<Command> command) { m_commands.push_back(std::move(command)); }
        const Command& GetNextCommand() const { return *m_commands.front(); }
        std::unique_ptr<Command> PopNextCommand()
        {
            m_isOnFirstCommand = false;
            std::unique_ptr<Command> command = std::move(m_commands.front());
            m_commands.pop_front();
            return command;
        }

        bool IsOnFirstCommand() const { return m_isOnFirstCommand; }
        bool IsComplete() const { return m_commands.empty(); }
        bool IsApplicableForInstallingSource() const { return m_operationType == PackageOperationType::Install || m_operationType == PackageOperationType::Upgrade; }
        PackageOperationType GetPackageOperationType() const { return m_operationType; }

    private:
        OrchestratorQueueItemState m_state = OrchestratorQueueItemState::NotQueued;
        std::unique_ptr<COMContext> m_context;
        wil::unique_event m_completedEvent{ wil::EventOptions::ManualReset };
        OrchestratorQueueItemId m_id;
        std::deque<std::unique_ptr<Command>> m_commands;
        bool m_isOnFirstCommand = true;
        OrchestratorQueue* m_currentQueue = nullptr;
        PackageOperationType m_operationType = PackageOperationType::None;
    };

    struct OrchestratorQueueItemFactory
    {
        // Create queue item for install/upgrade
        static std::unique_ptr<OrchestratorQueueItem> CreateItemForInstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context, bool isUpgrade);
        // Create queue item for uninstall
        static std::unique_ptr<OrchestratorQueueItem> CreateItemForUninstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context);
        // Create queue item for finding existing entry from the orchestrator queue
        static std::unique_ptr<OrchestratorQueueItem> CreateItemForSearch(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context);
    };

    struct ContextOrchestrator
    {
        ContextOrchestrator();
        static ContextOrchestrator& Instance();

        void EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> queueItem);
        void CancelQueueItem(const OrchestratorQueueItem& item);

        std::shared_ptr<OrchestratorQueueItem> GetQueueItem(const OrchestratorQueueItemId& queueItemId);

        void AddItemManifestToInstallingSource(const OrchestratorQueueItem& queueItem);
        void RemoveItemManifestFromInstallingSource(const OrchestratorQueueItem& queueItem);

    private:
        std::mutex m_queueLock;
        void AddCommandQueue(std::string_view commandName, UINT32 allowedThreads);
        void RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state);

        _Requires_lock_held_(m_queueLock)
        std::shared_ptr<OrchestratorQueueItem> FindById(const OrchestratorQueueItemId& queueItemId);

        Repository::Source m_installingWriteableSource;
        std::map<std::string, std::unique_ptr<OrchestratorQueue>> m_commandQueues;
    };

    // One of the queues used by the orchestrator.
    // All items in the queue execute the same command.
    // The queue allows multiple items to run at the same time, up to a limit.
    struct OrchestratorQueue
    {
        OrchestratorQueue(std::string_view commandName, UINT32 allowedThreads);
        ~OrchestratorQueue();

        // Name of the command this queue can execute
        std::string_view CommandName() const { return m_commandName; }

        // Enqueues an item to be run when there are threads available.
        void EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> item);

        // Removes an item by id, provided that it is in the given state.
        // Returns true if an item was removed.
        // The item can be removed globally from the orchestrator, or from just this queue.
        bool RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state, bool isGlobalRemove);

        // Finds an item by id, if it is in the queue.
        _Requires_lock_held_(m_queueLock)
        std::shared_ptr<OrchestratorQueueItem> FindById(const OrchestratorQueueItemId& queueItemId);

        // Runs a single item from the queue.
        void RunItem(const OrchestratorQueueItemId& itemId);

    private:
        // Enqueues an item.
        void EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item);

        _Requires_lock_held_(m_queueLock)
        std::deque<std::shared_ptr<OrchestratorQueueItem>>::iterator FindIteratorById(const OrchestratorQueueItemId& comparisonQueueItemId);

        std::string_view m_commandName;

        // Number of threads allowed to run items in this queue.
        const UINT32 m_allowedThreads;

        // Thread pool for this queue, and associated objects.
        // All work items will be added to the callback environment, and the cleanup group
        // will manage their closing.
        // See https://docs.microsoft.com/windows/win32/procthread/using-the-thread-pool-functions
        TP_CALLBACK_ENVIRON m_threadPoolCallbackEnviron;
        wil::unique_any<PTP_POOL, decltype(CloseThreadpool), CloseThreadpool> m_threadPool;
        wil::unique_any<PTP_CLEANUP_GROUP, decltype(CloseThreadpoolCleanupGroup), CloseThreadpoolCleanupGroup> m_threadPoolCleanupGroup;

        std::mutex m_queueLock;
        std::deque<std::shared_ptr<OrchestratorQueueItem>> m_queueItems;
    };
}
