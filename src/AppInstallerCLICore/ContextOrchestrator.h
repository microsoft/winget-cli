// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include "ExecutionReporter.h"
#include "ExecutionArgs.h"
#include "ExecutionContextData.h"
#include "CompletionData.h"
#include "COMContext.h"
#include "Commands/COMInstallCommand.h"

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
        const std::string& GetCurrentCommandName() const { return m_currentCommandName;  }
        Command& GetNextCommand() { return *m_commands.front();  }
        std::unique_ptr<Command> PopNextCommand() 
        { 
            std::unique_ptr<Command> command = std::move(m_commands.front());
            m_currentCommandName = command->NameAsString();
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
        std::string m_currentCommandName;
    };

    struct OrchestratorQueueItemFactory
    {
        static std::unique_ptr<OrchestratorQueueItem> CreateItemForInstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context);
    };

    struct ContextOrchestrator
    {
        ContextOrchestrator();
        static ContextOrchestrator& Instance();

        void EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> queueItem);
        void CancelQueueItem(const OrchestratorQueueItem& item);

        std::shared_ptr<OrchestratorQueueItem> GetQueueItem(const OrchestratorQueueItemId& queueItemId);

    private:
        std::mutex m_queueLock;
        void RunItems();
        std::shared_ptr<OrchestratorQueueItem> GetNextItem();
        void EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item);
        void RequeueItem(OrchestratorQueueItem& item);
        void RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state);

        _Requires_lock_held_(m_queueLock)
        std::deque<std::shared_ptr<OrchestratorQueueItem>>::iterator FindIteratorById(const OrchestratorQueueItemId& queueItemId);
        _Requires_lock_held_(m_queueLock)
        std::shared_ptr<OrchestratorQueueItem> FindById(const OrchestratorQueueItemId& queueItemId);

        std::shared_ptr<::AppInstaller::Repository::IMutablePackageSource> m_installingWriteableSource = nullptr;
        std::deque<std::shared_ptr<OrchestratorQueueItem>> m_queueItems;
        std::map<std::string, UINT32> m_runningCommandCountMap;
    };
}
