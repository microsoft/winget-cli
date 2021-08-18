// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "ContextOrchestrator.h"
#include "COMContext.h"
#include "Commands/COMInstallCommand.h"
#include "winget/UserSettings.h"
#include <Commands/RootCommand.h>

namespace AppInstaller::CLI::Execution
{
    ContextOrchestrator& ContextOrchestrator::Instance()
    {
        static ContextOrchestrator s_instance;
        return s_instance;
    }

    ContextOrchestrator::ContextOrchestrator()
    {
        ::AppInstaller::ProgressCallback progress;
        std::shared_ptr<::AppInstaller::Repository::ISource> installingSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installing, progress);
        m_installingWriteableSource = std::dynamic_pointer_cast<::AppInstaller::Repository::IMutablePackageSource>(installingSource);
    }

    _Requires_lock_held_(m_queueLock) 
    std::deque<std::shared_ptr<OrchestratorQueueItem>>::iterator ContextOrchestrator::FindIteratorById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        return std::find_if(m_queueItems.begin(), m_queueItems.end(), [&comparisonQueueItemId](const std::shared_ptr<OrchestratorQueueItem>& item) {return (item->GetId().IsSame(comparisonQueueItemId)); });

    }
    _Requires_lock_held_(m_queueLock)
    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::FindById(const OrchestratorQueueItemId& comparisonQueueItemId)
    {
        auto itr = FindIteratorById(comparisonQueueItemId);
        if (itr != m_queueItems.end())
        {
            return *itr;
        }
        return {};
    }
    
    void ContextOrchestrator::EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSTALL_ALREADY_RUNNING), FindById(item->GetId()));
        m_queueItems.push_back(item);

        // Add the package to the Installing source so that it can be queried using the ISource interface.
        const auto& manifest = item->GetContext().Get<Execution::Data::Manifest>();
        m_installingWriteableSource->AddPackageVersion(manifest, std::filesystem::path{ manifest.Id + '.' + manifest.Version });
    }

    void ContextOrchestrator::EnqueueAndRunItem(std::shared_ptr<OrchestratorQueueItem> item)
    {
        EnqueueItem(item);

        std::thread runnerThread(&ContextOrchestrator::RunItems, this);
        runnerThread.detach();
    }

    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::GetNextItem()
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        if (m_queueItems.empty())
        {
            return {};
        }

        std::shared_ptr<OrchestratorQueueItem> item = m_queueItems.front();

        // Check if item can be dequeued.
        // Since only one item can be installed at a time currently the logic is very simple,
        // and can just check if the first item is already running. This logic will need to become
        // more complicated if multiple operation types (e.g. Download & Install) are added that can
        // run simultaneously.
        if (item->GetState() == OrchestratorQueueItemState::Running)
        {
            return {};
        }

        // Running state must be set inside the queueLock so that multiple threads don't try to run the same item.
        item->SetState(OrchestratorQueueItemState::Running);
        return item;
    }

    void ContextOrchestrator::RunItems()
    {
        std::shared_ptr<OrchestratorQueueItem> item = GetNextItem();
        while(item != nullptr)
        {
            HRESULT terminationHR = S_OK;
            try
            {
                ::AppInstaller::CLI::RootCommand rootCommand;
                std::unique_ptr<::AppInstaller::CLI::Command> command = std::make_unique<::AppInstaller::CLI::COMInstallCommand>(rootCommand.Name());
                ::AppInstaller::Logging::Telemetry().LogCommand(command->FullName());
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

            RemoveItemInState(*item, OrchestratorQueueItemState::Running);

            item = GetNextItem();
        }
    }

    void ContextOrchestrator::RemoveItemInState(const OrchestratorQueueItem& item, OrchestratorQueueItemState state)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        // Look for the item. It's ok if the item is not found since multiple listeners may try to remove the same item.
        //auto itr = std::find(m_queueItems.begin(), m_queueItems.end(), item);
        auto itr = FindIteratorById(item.GetId());
        if (itr != m_queueItems.end() && (*itr)->GetState() == state)
        {
            m_queueItems.erase(itr);

            const auto& manifest = item.GetContext().Get<Execution::Data::Manifest>();
            m_installingWriteableSource->RemovePackageVersion(manifest, std::filesystem::path{ manifest.Id + '.' + manifest.Version });

            item.GetCompletedEvent().SetEvent();
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

    bool OrchestratorQueueItemId::IsSame(const OrchestratorQueueItemId& comparedId) const
    {
        return ((GetPackageId() == comparedId.GetPackageId()) && 
                (GetSourceId() == comparedId.GetSourceId()));
    }

    std::unique_ptr<OrchestratorQueueItem> OrchestratorQueueItemFactory::CreateItemForInstall(std::wstring packageId, std::wstring sourceId, std::unique_ptr<COMContext> context)
    {
        return std::make_unique<OrchestratorQueueItem>(OrchestratorQueueItemId(std::move(packageId), std::move(sourceId)), std::move(context));
    }

}
