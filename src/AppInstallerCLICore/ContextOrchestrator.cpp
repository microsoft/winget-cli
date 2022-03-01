// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "ContextOrchestrator.h"
#include "COMContext.h"
#include "Commands/COMCommand.h"
#include "winget/UserSettings.h"
#include <Commands/RootCommand.h>

namespace AppInstaller::CLI::Execution
{
    namespace
    {
        // Operation command queue used by install and uninstall commands.
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
            if (commandName == COMInstallCommand::CommandName || commandName == COMUninstallCommand::CommandName)
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

    ContextOrchestrator::ContextOrchestrator()
    {
        ProgressCallback progress;
        m_installingWriteableSource = Repository::Source(Repository::PredefinedSource::Installing);
        m_installingWriteableSource.Open(progress);

        // Decide how many threads to use for each command.
        // We always allow only one install at a time.
        // For download, if we can find the number of supported concurrent threads,
        // use that as the maximum (up to 3); otherwise use a single thread.
        const auto supportedConcurrentThreads = std::thread::hardware_concurrency();
        const UINT32 maxDownloadThreads = 3;
        const UINT32 operationThreads = 1;
        const UINT32 downloadThreads = std::min(supportedConcurrentThreads ? supportedConcurrentThreads - 1 : 1, maxDownloadThreads);

        AddCommandQueue(COMDownloadCommand::CommandName, downloadThreads);
        AddCommandQueue(OperationCommandQueueName, operationThreads);
    }

    void ContextOrchestrator::AddCommandQueue(std::string_view commandName, UINT32 allowedThreads)
    {
        m_commandQueues.emplace(commandName, std::make_unique<OrchestratorQueue>(commandName, allowedThreads));
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

    void ContextOrchestrator::EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> item)
    {
        std::lock_guard<std::mutex> lockQueue{ m_queueLock };

        if (item->IsOnFirstCommand())
        {
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSTALL_ALREADY_RUNNING), FindById(item->GetId()));
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
        item.GetContext().Cancel(false, true);

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

    _Requires_lock_held_(m_queueLock)
    std::deque<std::shared_ptr<OrchestratorQueueItem>>::iterator OrchestratorQueue::FindIteratorById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        return std::find_if(m_queueItems.begin(), m_queueItems.end(), [&comparisonQueueItemId](const std::shared_ptr<OrchestratorQueueItem>& item) {return (item->GetId().IsSame(comparisonQueueItemId)); });
    }

    _Requires_lock_held_(m_queueLock)
    std::shared_ptr<OrchestratorQueueItem> OrchestratorQueue::FindById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        auto itr = FindIteratorById(comparisonQueueItemId);
        if (itr != m_queueItems.end())
        {
            return *itr;
        }

        return {};
    }

    void OrchestratorQueue::EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item)
    {
        {
            std::lock_guard<std::mutex> lockQueue{ m_queueLock };
            m_queueItems.push_back(item);
        }

        // Add the package to the Installing source so that it can be queried using the Source interface.
        // Only do this the first time the item is queued.
        if (item->IsOnFirstCommand())
        {
            ContextOrchestrator::Instance().AddItemManifestToInstallingSource(*item);
        }

        {
            std::lock_guard<std::mutex> lockQueue{ m_queueLock };
            item->SetState(OrchestratorQueueItemState::Queued);
        }
    }

    OrchestratorQueue::OrchestratorQueue(std::string_view commandName, UINT32 allowedThreads) :
        m_commandName(commandName), m_allowedThreads(allowedThreads)
    {
        m_threadPool.reset(CreateThreadpool(nullptr));
        THROW_LAST_ERROR_IF_NULL(m_threadPool);
        m_threadPoolCleanupGroup.reset(CreateThreadpoolCleanupGroup());
        THROW_LAST_ERROR_IF_NULL(m_threadPoolCleanupGroup);
        InitializeThreadpoolEnvironment(&m_threadPoolCallbackEnviron);
        SetThreadpoolCallbackPool(&m_threadPoolCallbackEnviron, m_threadPool.get());
        SetThreadpoolCallbackCleanupGroup(&m_threadPoolCallbackEnviron, m_threadPoolCleanupGroup.get(), nullptr);

        THROW_LAST_ERROR_IF(!SetThreadpoolThreadMinimum(m_threadPool.get(), 1));
        SetThreadpoolThreadMaximum(m_threadPool.get(), m_allowedThreads);
    }

    OrchestratorQueue::~OrchestratorQueue()
    {
        CloseThreadpoolCleanupGroupMembers(m_threadPoolCleanupGroup.get(), false, nullptr);
    }

    void OrchestratorQueue::EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> item)
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
                std::lock_guard<std::mutex> lockQueue{ m_queueLock };
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
            HRESULT terminationHR = S_OK;
            try
            {
                std::unique_ptr<Command> command = item->PopNextCommand();

                std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> setThreadGlobalsToPreviousState = item->GetContext().SetForCurrentThread();

                item->GetContext().GetThreadGlobals().GetTelemetryLogger().LogCommand(command->FullName());
                command->ValidateArguments(item->GetContext().Args);

                item->GetContext().EnableCtrlHandler();

                terminationHR = ::AppInstaller::CLI::Execute(item->GetContext(), command);
            }
            WINGET_CATCH_STORE(terminationHR, APPINSTALLER_CLI_ERROR_COMMAND_FAILED);

            if (FAILED(terminationHR))
            {
                // ::Execute sometimes catches exceptions and returns hresults based on those exceptions without the context
                // being updated with that hresult. This sets the termination hr directly so that the context always 
                // has the result of the operation no matter how it failed.
                item->GetContext().SetTerminationHR(terminationHR);
            }

            item->GetContext().EnableCtrlHandler(false);

            if (FAILED(terminationHR) || item->IsComplete())
            {
                RemoveItemInState(*item, OrchestratorQueueItemState::Running, true);
            }
            else
            {
                // Remove item from this queue and add it to the queue for the next command.
                RemoveItemInState(*item, OrchestratorQueueItemState::Running, false);
                ContextOrchestrator::Instance().EnqueueAndRunItem(item);
            }
        }
        catch (...)
        {
        }
    }

    bool OrchestratorQueue::RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state, bool isGlobalRemove)
    {
        // OrchestratorQueueItemState::Running items should only be removed by the thread that ran the item.
        // Queued items can be removed by any thread.
        // NotQueued items should not be removed since, if found in the queue, they are in the process of being queued by another thread.
        bool foundItem = false;

        {
            std::lock_guard<std::mutex> lockQueue{ m_queueLock };

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
                }
                else if (state == OrchestratorQueueItemState::Queued)
                {
                    (*itr)->SetState(OrchestratorQueueItemState::Cancelled);
                }
            }
        }

        if (foundItem && isGlobalRemove)
        {
            ContextOrchestrator::Instance().RemoveItemManifestFromInstallingSource(item);
            item.GetCompletedEvent().SetEvent();
        }

        return foundItem;
    }

    bool OrchestratorQueueItemId::IsSame(const OrchestratorQueueItemId& comparedId) const
    {
        return ((GetPackageId() == comparedId.GetPackageId()) && 
                (GetSourceId() == comparedId.GetSourceId()));
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
        std::unique_ptr<OrchestratorQueueItem> item = std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context), PackageOperationType::None);
        return item;
    }
}
