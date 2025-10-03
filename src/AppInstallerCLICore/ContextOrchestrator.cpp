// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "ContextOrchestrator.h"
#include "COMContext.h"
#include "Commands/COMCommand.h"
#include "Public/ShutdownMonitoring.h"
#include "winget/UserSettings.h"
#include <Commands/RootCommand.h>

namespace AppInstaller::CLI::Execution
{
    namespace
    {
        // Operation command queue used by install, uninstall and repair commands.
        constexpr static std::string_view OperationCommandQueueName = "operation"sv;

        // Callback function used by worker threads in the queue.
        // context must be a pointer to a queue item.
        void CALLBACK OrchestratorQueueWorkCallback(PTP_CALLBACK_INSTANCE, PVOID context, PTP_WORK)
        {
            auto queueItem = reinterpret_cast<OrchestratorQueueItem*>(context);
            auto queue = queueItem->GetCurrentQueue();
            if (queue)
            {
                queue->RunItem(queueItem->GetId());
            }
        }

        // Get command queue name based on command name.
        std::string_view GetCommandQueueName(std::string_view commandName)
        {
            if (commandName == COMInstallCommand::CommandName || commandName == COMUninstallCommand::CommandName || commandName == COMRepairCommand::CommandName)
            {
                return OperationCommandQueueName;
            }

            return commandName;
        }
    }

    ContextOrchestrator& ContextOrchestrator::Instance()
    {
        static ContextOrchestrator s_instance;
        return s_instance;
    }

    ContextOrchestrator::ContextOrchestrator() : ContextOrchestrator(std::thread::hardware_concurrency()) {}

    ContextOrchestrator::ContextOrchestrator(unsigned int hardwareConcurrency)
    {
        ProgressCallback progress;
        m_installingWriteableSource = Repository::Source(Repository::PredefinedSource::Installing);
        m_installingWriteableSource.Open(progress);

        // Decide how many threads to use for each command.
        // We always allow only one install at a time.
        // For download, if we can find the number of supported concurrent threads,
        // use that as the maximum (up to 3); otherwise use a single thread.
        const UINT32 maxDownloadThreads = 3;
        const UINT32 operationThreads = 1;
        const UINT32 downloadThreads = std::min(hardwareConcurrency > 1 ? hardwareConcurrency - 1 : 1, maxDownloadThreads);

        AddCommandQueue(COMDownloadCommand::CommandName, downloadThreads);
        AddCommandQueue(OperationCommandQueueName, operationThreads);
    }

    void ContextOrchestrator::AddCommandQueue(std::string_view commandName, UINT32 allowedThreads)
    {
        std::lock_guard<std::mutex> lockQueue{ m_queueLock };
        m_commandQueues.emplace(commandName, std::make_unique<OrchestratorQueue>(*this, commandName, allowedThreads));
    }

    _Requires_lock_held_(m_queueLock)
    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::FindById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        for (const auto& queue : m_commandQueues)
        {
            auto item = queue.second->FindById(comparisonQueueItemId);
            if (item)
            {
                return item;
            }
        }

        return {};
    }

    void ContextOrchestrator::EnqueueAndRunItem(const std::shared_ptr<OrchestratorQueueItem>& item)
    {
        std::lock_guard<std::mutex> lockQueue{ m_queueLock };

        if (item->IsOnFirstCommand())
        {
            // Directly error on attempting to enqueue first time
            THROW_HR_IF(ToHRESULT(m_disabledReason), !m_enabled);

            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSTALL_ALREADY_RUNNING), FindById(item->GetId()));

            // Log the beginning of the item
            item->GetContext().GetThreadGlobals().GetTelemetryLogger().LogCommand(item->GetItemCommandName());
        }
        else if (!m_enabled)
        {
            // On subsequent command enqueues, cancel and complete the item
            item->GetContext().Cancel(m_disabledReason, true);
            item->HandleItemCompletion(*this);
        }

        std::string commandQueueName{ GetCommandQueueName(item->GetNextCommand().Name()) };
        m_commandQueues.at(commandQueueName)->EnqueueAndRunItem(item);
    }

    void ContextOrchestrator::RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state)
    {
        std::lock_guard<std::mutex> lockQueue{ m_queueLock };
        for (const auto& queue : m_commandQueues)
        {
            if (queue.second->RemoveItemInState(item, state, true))
            {
                return;
            }
        }
    }

    void ContextOrchestrator::CancelQueueItem(const OrchestratorQueueItem& item)
    {
        // Always cancel the item, even if it isn't running yet, to get the terminationHR set correctly.
        item.GetContext().Cancel(CancelReason::Abort, true);

        RemoveItemInState(item, OrchestratorQueueItemState::Queued);
    }

    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::GetQueueItem(const OrchestratorQueueItemId& queueItemId)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        return FindById(queueItemId);
    }

    void ContextOrchestrator::AddItemManifestToInstallingSource(const OrchestratorQueueItem& queueItem)
    {
        if (queueItem.IsApplicableForInstallingSource())
        {
            const auto& manifest = queueItem.GetContext().Get<Execution::Data::Manifest>();
            m_installingWriteableSource.AddPackageVersion(manifest, std::filesystem::path{ manifest.Id + '.' + manifest.Version });
        }
    }

    void ContextOrchestrator::RemoveItemManifestFromInstallingSource(const OrchestratorQueueItem& queueItem)
    {
        if (queueItem.IsApplicableForInstallingSource())
        {
            const auto& manifest = queueItem.GetContext().Get<Execution::Data::Manifest>();
            m_installingWriteableSource.RemovePackageVersion(manifest, std::filesystem::path{ manifest.Id + '.' + manifest.Version });
        }
    }

    void ContextOrchestrator::RegisterForShutdownSynchronization()
    {
        static std::once_flag registerComponentOnceFlag;
        std::call_once(registerComponentOnceFlag,
            [&]()
            {
                using namespace ShutdownMonitoring;

                ServerShutdownSynchronization::ComponentSystem component;
                component.BlockNewWork = StaticDisable;
                component.BeginShutdown = StaticCancelQueuedItems;
                component.Wait = StaticWaitForRunningItems;

                ServerShutdownSynchronization::AddComponent(component);
            });
    }

    void ContextOrchestrator::StaticDisable(CancelReason reason)
    {
        Instance().Disable(reason);
    }

    void ContextOrchestrator::StaticCancelQueuedItems(CancelReason reason)
    {
        Instance().CancelQueuedItems(reason);
    }

    void ContextOrchestrator::StaticWaitForRunningItems()
    {
        Instance().WaitForRunningItems();
    }

    void ContextOrchestrator::Disable(CancelReason reason)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };
        m_enabled = false;
        m_disabledReason = reason;
    }

    void ContextOrchestrator::CancelQueuedItems(CancelReason reason)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };
        for (const auto& queue : m_commandQueues)
        {
            queue.second->CancelAllItems(reason);
        }
    }

    void ContextOrchestrator::WaitForRunningItems()
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };
        for (const auto& queue : m_commandQueues)
        {
            queue.second->WaitForEmptyQueue();
        }
    }

    bool ContextOrchestrator::WaitForRunningItems(DWORD timeoutMilliseconds)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };
        for (const auto& queue : m_commandQueues)
        {
            if (!queue.second->WaitForEmptyQueue(timeoutMilliseconds))
            {
                return false;
            }
        }

        return true;
    }

    std::string ContextOrchestrator::GetStatusString()
    {
        std::ostringstream stream;

        std::lock_guard<std::mutex> lock{ m_queueLock };

        if (!m_enabled)
        {
            stream << "Disabled due to " << ToIntegral(m_disabledReason) << std::endl;
        }

        for (const auto& queue : m_commandQueues)
        {
            stream << queue.second->GetStatusString();
        }

        return stream.str();
    }

    _Requires_lock_held_(m_itemLock)
    std::deque<std::shared_ptr<OrchestratorQueueItem>>::iterator OrchestratorQueue::FindIteratorById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        return std::find_if(m_queueItems.begin(), m_queueItems.end(), [&comparisonQueueItemId](const std::shared_ptr<OrchestratorQueueItem>& item) {return (item->GetId().IsSame(comparisonQueueItemId)); });
    }

    _Requires_lock_held_(m_itemLock)
    std::shared_ptr<OrchestratorQueueItem> OrchestratorQueue::FindById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        auto itr = FindIteratorById(comparisonQueueItemId);
        if (itr != m_queueItems.end())
        {
            return *itr;
        }

        return {};
    }

    void OrchestratorQueue::EnqueueItem(const std::shared_ptr<OrchestratorQueueItem>& item)
    {
        {
            std::lock_guard<std::mutex> lockQueue{ m_itemLock };
            m_queueItems.push_back(item);
            m_queueEmpty.ResetEvent();
        }

        // Add the package to the Installing source so that it can be queried using the Source interface.
        // Only do this the first time the item is queued.
        if (item->IsOnFirstCommand())
        {
            try
            {
                m_orchestrator.AddItemManifestToInstallingSource(*item);
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lockQueue{ m_itemLock };
                auto itr = FindIteratorById(item->GetId());
                if (itr != m_queueItems.end())
                {
                    m_queueItems.erase(itr);

                    if (m_queueItems.empty())
                    {
                        m_queueEmpty.SetEvent();
                    }
                }
                throw;
            }
        }

        {
            std::lock_guard<std::mutex> lockQueue{ m_itemLock };
            item->SetState(OrchestratorQueueItemState::Queued);
        }
    }

    OrchestratorQueue::OrchestratorQueue(ContextOrchestrator& orchestrator, std::string_view commandName, UINT32 allowedThreads) :
        m_orchestrator(orchestrator), m_commandName(commandName), m_allowedThreads(allowedThreads)
    {
        m_threadPool.reset(CreateThreadpool(nullptr));
        THROW_LAST_ERROR_IF_NULL(m_threadPool);
        m_threadPoolCleanupGroup.reset(CreateThreadpoolCleanupGroup());
        THROW_LAST_ERROR_IF_NULL(m_threadPoolCleanupGroup);
        InitializeThreadpoolEnvironment(&m_threadPoolCallbackEnviron);
        SetThreadpoolCallbackPool(&m_threadPoolCallbackEnviron, m_threadPool.get());
        SetThreadpoolCallbackCleanupGroup(&m_threadPoolCallbackEnviron, m_threadPoolCleanupGroup.get(), nullptr);

        SetThreadpoolThreadMaximum(m_threadPool.get(), m_allowedThreads);
        THROW_LAST_ERROR_IF(!SetThreadpoolThreadMinimum(m_threadPool.get(), 1));
    }

    OrchestratorQueue::~OrchestratorQueue()
    {
        CloseThreadpoolCleanupGroupMembers(m_threadPoolCleanupGroup.get(), false, nullptr);
    }

    void OrchestratorQueue::EnqueueAndRunItem(const std::shared_ptr<OrchestratorQueueItem>& item)
    {
        EnqueueItem(item);

        item->SetCurrentQueue(this);
        auto work = CreateThreadpoolWork(OrchestratorQueueWorkCallback, item.get(), &m_threadPoolCallbackEnviron);
        SubmitThreadpoolWork(work);
    }

    void OrchestratorQueue::RunItem(const OrchestratorQueueItemId& itemId)
    {
        try
        {
            std::shared_ptr<OrchestratorQueueItem> item;
            bool isCancelled = false;

            // Try to find the item in the queue.
            {
                std::lock_guard<std::mutex> lockQueue{ m_itemLock };
                item = FindById(itemId);

                if (!item)
                {
                    // Item should be in the queue; this shouldn't happen.
                    return;
                }

                // Only run if the item is queued and not cancelled.
                if (item->GetState() == OrchestratorQueueItemState::Queued)
                {
                    // Mark it as running so that it cannot be cancelled by other threads.
                    item->SetState(OrchestratorQueueItemState::Running);
                }
                else if (item->GetState() == OrchestratorQueueItemState::Cancelled)
                {
                    isCancelled = true;
                }
            }

            if (isCancelled)
            {
                // Do this separate from above block as the Remove function needs to manage the lock.
                RemoveItemInState(*item, OrchestratorQueueItemState::Cancelled, true);
            }

            // Get the item's command and execute it.
            HRESULT exceptionHR = S_OK;
            try
            {
                std::unique_ptr<Command> command = item->PopNextCommand();

                std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> setThreadGlobalsToPreviousState = item->GetContext().SetForCurrentThread();

                command->ValidateArguments(item->GetContext().Args);

                item->GetContext().EnableSignalTerminationHandler();

                ::AppInstaller::CLI::ExecuteWithoutLoggingSuccess(item->GetContext(), command.get());
            }
            WINGET_CATCH_STORE(exceptionHR, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);

            if (FAILED(exceptionHR))
            {
                // Set the termination hr directly from any exception that escaped so that the context always 
                // has the result of the operation no matter how it failed.
                item->GetContext().SetTerminationHR(exceptionHR);
            }

            item->GetContext().EnableSignalTerminationHandler(false);

            if (FAILED(item->GetContext().GetTerminationHR()) || item->IsComplete())
            {
                if (SUCCEEDED(item->GetContext().GetTerminationHR()))
                {
                    item->GetContext().GetThreadGlobals().GetTelemetryLogger().LogCommandSuccess(item->GetItemCommandName());
                }

                RemoveItemInState(*item, OrchestratorQueueItemState::Running, true);
            }
            else
            {
                // Remove item from this queue and add it to the queue for the next command.
                RemoveItemInState(*item, OrchestratorQueueItemState::Running, false);
                m_orchestrator.EnqueueAndRunItem(item);
            }
        }
        catch (...)
        {
        }
    }

    void OrchestratorQueue::CancelAllItems(CancelReason reason)
    {
        std::lock_guard<std::mutex> lockQueue{ m_itemLock };

        for (auto itr = m_queueItems.begin(); itr != m_queueItems.end(); itr++)
        {
            auto& item = *itr;

            item->GetContext().Cancel(reason, true);

            // This mimics ContextOrchestrator::CancelQueueItem, which speeds up the process of cancelling queued items
            if (item->GetState() == OrchestratorQueueItemState::Queued)
            {
                item->SetState(OrchestratorQueueItemState::Cancelled);
                item->HandleItemCompletion(m_orchestrator);
            }
        }
    }

    void OrchestratorQueue::WaitForEmptyQueue()
    {
        m_queueEmpty.wait();
    }

    bool OrchestratorQueue::WaitForEmptyQueue(DWORD timeoutMilliseconds)
    {
        return m_queueEmpty.wait(timeoutMilliseconds);
    }

    std::string OrchestratorQueue::GetStatusString()
    {
        std::ostringstream stream;
        stream << m_commandName << '[' << m_allowedThreads << "]\n";

        std::map<OrchestratorQueueItemState, size_t> stateCounts;
        stateCounts[OrchestratorQueueItemState::NotQueued] = 0;
        stateCounts[OrchestratorQueueItemState::Queued] = 0;
        stateCounts[OrchestratorQueueItemState::Running] = 0;
        stateCounts[OrchestratorQueueItemState::Cancelled] = 0;

        {
            std::lock_guard<std::mutex> lock{ m_itemLock };

            for (const auto& item : m_queueItems)
            {
                stateCounts[item->GetState()] += 1;
            }
        }

        for (const auto& stateCount : stateCounts)
        {
            stream << "  " << ToString(stateCount.first) << " : " << stateCount.second << std::endl;
        }

        return stream.str();
    }

    bool OrchestratorQueue::RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state, bool isGlobalRemove)
    {
        // OrchestratorQueueItemState::Running items should only be removed by the thread that ran the item.
        // Queued items can be removed by any thread.
        // NotQueued items should not be removed since, if found in the queue, they are in the process of being queued by another thread.
        bool foundItem = false;

        {
            std::lock_guard<std::mutex> lockQueue{ m_itemLock };

            // Look for the item. It's ok if the item is not found since multiple listeners may try to remove the same item.
            auto itr = FindIteratorById(item.GetId());
            if (itr != m_queueItems.end() && (*itr)->GetState() == state)
            {
                foundItem = true;

                // The item must only be removed from the queue by the thread that runs
                // it, because the callback uses it. If any other thread tries to remove
                // it, we simply mark it as cancelled.
                if (state == OrchestratorQueueItemState::Running || state == OrchestratorQueueItemState::Cancelled)
                {
                    (*itr)->SetCurrentQueue(nullptr);
                    m_queueItems.erase(itr);

                    if (m_queueItems.empty())
                    {
                        m_queueEmpty.SetEvent();
                    }
                }
                else if (state == OrchestratorQueueItemState::Queued)
                {
                    (*itr)->SetState(OrchestratorQueueItemState::Cancelled);
                }
            }
        }

        if (foundItem && isGlobalRemove)
        {
            item.HandleItemCompletion(m_orchestrator);
        }

        return foundItem;
    }

    bool OrchestratorQueueItemId::IsSame(const OrchestratorQueueItemId& comparedId) const
    {
        return ((GetPackageId() == comparedId.GetPackageId()) && 
                (GetSourceId() == comparedId.GetSourceId()));
    }

    std::string_view OrchestratorQueueItem::GetItemCommandName() const
    {
        // The goal is that these should match the winget.exe commands for easy correlation.
        switch (m_operationType)
        {
        case PackageOperationType::Search: return "root:search"sv;
        case PackageOperationType::Install: return "root:install"sv;
        case PackageOperationType::Upgrade: return "root:upgrade"sv;
        case PackageOperationType::Uninstall: return "root:uninstall"sv;
        case PackageOperationType::Download: return "root:download"sv;
        case PackageOperationType::Repair: return "root:repair"sv;
        default: return "unknown";
        }
    }

    void OrchestratorQueueItem::HandleItemCompletion(ContextOrchestrator& orchestrator) const
    {
        orchestrator.RemoveItemManifestFromInstallingSource(*this);
        GetCompletedEvent().SetEvent();
    }

    std::unique_ptr<OrchestratorQueueItem> OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context, bool isUpgrade)
    {
        std::unique_ptr<OrchestratorQueueItem> item = std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context), isUpgrade ? PackageOperationType::Upgrade : PackageOperationType::Install);
        item->AddCommand(std::make_unique<::AppInstaller::CLI::COMDownloadCommand>(RootCommand::CommandName));
        item->AddCommand(std::make_unique<::AppInstaller::CLI::COMInstallCommand>(RootCommand::CommandName));
        return item;
    }

    std::unique_ptr<OrchestratorQueueItem> OrchestratorQueueItemFactory::CreateItemForUninstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context)
    {
        std::unique_ptr<OrchestratorQueueItem> item = std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context), PackageOperationType::Uninstall);
        item->AddCommand(std::make_unique<::AppInstaller::CLI::COMUninstallCommand>(RootCommand::CommandName));
        return item;
    }

    std::unique_ptr<OrchestratorQueueItem> OrchestratorQueueItemFactory::CreateItemForSearch(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context)
    {
        std::unique_ptr<OrchestratorQueueItem> item = std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context), PackageOperationType::Search);
        return item;
    }

    std::unique_ptr<OrchestratorQueueItem> OrchestratorQueueItemFactory::CreateItemForDownload(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context)
    {
        std::unique_ptr<OrchestratorQueueItem> item = std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context), PackageOperationType::Download);
        item->AddCommand(std::make_unique<::AppInstaller::CLI::COMDownloadCommand>(RootCommand::CommandName));
        return item;
    }

    std::unique_ptr<OrchestratorQueueItem> OrchestratorQueueItemFactory::CreateItemForRepair(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context)
    {
        std::unique_ptr<OrchestratorQueueItem> item = std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context), PackageOperationType::Repair);
        item->AddCommand(std::make_unique<::AppInstaller::CLI::COMRepairCommand>(RootCommand::CommandName));
        return item;
    }

    std::string_view ToString(OrchestratorQueueItemState state)
    {
        switch (state)
        {
        case OrchestratorQueueItemState::NotQueued: return "NotQueued";
        case OrchestratorQueueItemState::Queued: return "Queued";
        case OrchestratorQueueItemState::Running: return "Running";
        case OrchestratorQueueItemState::Cancelled: return "Cancelled";
        default: return "Unknown";
        }
    }
}
