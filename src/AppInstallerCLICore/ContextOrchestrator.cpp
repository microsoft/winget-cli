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

    std::string_view ContextOrchestrator::GetIdFromItem(const std::shared_ptr<OrchestratorQueueItem>& item)
    {
        return item->GetContext()->Args.GetArg(::AppInstaller::CLI::Execution::Args::Type::Id);
    }

    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::FindById(std::string_view packageId)
    {
        auto it = std::find_if(m_queueItems.begin(), m_queueItems.end(), [&packageId](const std::shared_ptr<OrchestratorQueueItem> item) {return (GetIdFromItem(item) == packageId); });
        if (it != m_queueItems.end())
        {
            return *it;
        }
        return {};
    }

    void ContextOrchestrator::CreateInstallingWriteableSource()
    {
        // Create the Installing source, which will contain the set of packages that the ContextOrchestrator is currently installing.
        std::call_once(m_installingWriteableSourceOnceFlag,
            [&]()
            {
                ::AppInstaller::ProgressCallback progress;
                std::shared_ptr<::AppInstaller::Repository::ISource> installingSource = ::AppInstaller::Repository::OpenPredefinedSource(::AppInstaller::Repository::PredefinedSource::Installing, progress);
                m_installingWriteableSource = std::dynamic_pointer_cast<::AppInstaller::Repository::IMutablePackageSource>(installingSource);
                if (!m_installingWriteableSource)
                {
                    throw E_UNEXPECTED;
                }
            });
    }

    void ContextOrchestrator::EnqueueItem(std::shared_ptr<OrchestratorQueueItem> item)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INSTALL_ALREADY_RUNNING), FindById(GetIdFromItem(item)));
        m_queueItems.push_back(item);

        // Add the package to the Installing source so that it can be queried using the ISource interface.
        const auto& manifest = item->GetContext()->Get<Execution::Data::Manifest>();
        m_installingWriteableSource->AddPackageVersion(manifest, std::filesystem::path{ manifest.Id + '.' + manifest.Version });
    }

    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::EnqueueAndRunCommand(std::shared_ptr<COMContext> context)
    {
        CreateInstallingWriteableSource();

        wil::unique_handle completedEvent(::CreateEvent(nullptr, true, false, nullptr));
        THROW_LAST_ERROR_IF(!completedEvent);

        std::shared_ptr<OrchestratorQueueItem> item = std::make_shared<OrchestratorQueueItem>(OrchestratorQueueItemState::Queued, context, completedEvent.release());
        EnqueueItem(item);

        std::thread runnerThread(&ContextOrchestrator::RunContexts, this);
        runnerThread.detach();

        return item;
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

    void ContextOrchestrator::RunContexts()
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
                rootCommand.ValidateArguments(item->GetContext()->Args);

                item->GetContext()->EnableCtrlHandler();

                terminationHR = ::AppInstaller::CLI::Execute(*item->GetContext(), command);
            }
            WINGET_CATCH_STORE(terminationHR);

            if (FAILED(terminationHR))
            {
                item->GetContext()->Terminate(terminationHR);
            }

            RemoveItemInState(item, OrchestratorQueueItemState::Running);

            item = GetNextItem();
        }
    }

    void ContextOrchestrator::RemoveItemInState(const std::shared_ptr<OrchestratorQueueItem>& item, OrchestratorQueueItemState state)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        // Look for the item. It's ok if the item is not found since multiple listeners may try to remove the same item.
        auto itr = std::find(m_queueItems.begin(), m_queueItems.end(), item);
        if (itr != m_queueItems.end() && (*itr)->GetState() == state)
        {
            m_queueItems.erase(itr);

            const auto& manifest = item->GetContext()->Get<Execution::Data::Manifest>();
            m_installingWriteableSource->RemovePackageVersion(manifest, std::filesystem::path{ manifest.Id + '.' + manifest.Version });

            ::SetEvent(item->GetCompletedEvent());
        }
    }
    void ContextOrchestrator::CancelItem(const std::shared_ptr<OrchestratorQueueItem>& item)
    {
        // Always cancel the item, even if it isn't running yet, to get the terminationHR set correctly.
        item->GetContext()->Cancel(false, true);

        RemoveItemInState(item, OrchestratorQueueItemState::Queued);
    }

    std::shared_ptr<OrchestratorQueueItem> ContextOrchestrator::GetQueueItem(std::string id)
    {
        std::lock_guard<std::mutex> lock{ m_queueLock };

        return FindById(id);
    }

}
