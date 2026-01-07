// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerRuntime.h"
#include "Argument.h"
#include "COMContext.h"
#include "Command.h"
#include "ExecutionContext.h"
#include "Public/ShutdownMonitoring.h"
#include <winget/Checkpoint.h>
#include <winget/Reboot.h>
#include <winget/UserSettings.h>
#include <winget/NetworkSettings.h>

using namespace AppInstaller::Checkpoints;

namespace AppInstaller::CLI::Execution
{
    using namespace Settings;

    namespace
    {
        bool ShouldRemoveCheckpointDatabase(HRESULT hr)
        {
            switch (hr)
            {
            case APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_FOR_INSTALL:
            case APPINSTALLER_CLI_ERROR_RESUME_LIMIT_EXCEEDED:
            case APPINSTALLER_CLI_ERROR_CLIENT_VERSION_MISMATCH:
                return false;
            default:
                return true;
            }
        }
    }

    Context::~Context()
    {
        if (Settings::ExperimentalFeature::IsEnabled(ExperimentalFeature::Feature::Resume))
        {
            if (m_checkpointManager && (!IsTerminated() || ShouldRemoveCheckpointDatabase(GetTerminationHR())))
            {
                m_checkpointManager->CleanUpDatabase();
                AppInstaller::Reboot::UnregisterRestartForWER();
            }
        }

        if (m_disableSignalTerminationHandlerOnExit)
        {
            EnableSignalTerminationHandler(false);
        }
    }

    Context Context::CreateEmptyContext()
    {
        AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
        return Context(Reporter, threadGlobals);
    }

    std::unique_ptr<Context> Context::CreateSubContext()
    {
        auto clone = std::make_unique<Context>(Reporter, *m_threadGlobals);
        clone->m_flags = m_flags;
        clone->m_executingCommand = m_executingCommand;
        // If the parent is hooked up to the CTRL signal, have the clone be as well
        if (m_disableSignalTerminationHandlerOnExit)
        {
            clone->EnableSignalTerminationHandler();
        }
        CopyArgsToSubContext(clone.get());
        CopyDataToSubContext(clone.get());
        return clone;
    }

    void Context::CopyArgsToSubContext(Context* subContext)
    {
        auto argProperties = ArgumentCommon::GetFromExecArgs(Args);
        for (const auto& arg : argProperties)
        {
            if (WI_IsFlagSet(arg.TypeCategory, ArgTypeCategory::CopyFlagToSubContext))
            {
                subContext->Args.AddArg(arg.Type);
            }
            else if (WI_IsFlagSet(arg.TypeCategory, ArgTypeCategory::CopyValueToSubContext))
            {
                subContext->Args.AddArg(arg.Type, Args.GetArg(arg.Type));
            }
        }
    }

    void Context::CopyDataToSubContext(Context* subContext)
    {
#define COPY_DATA_IF_EXISTS(dataType) \
        if (this->Contains(dataType)) \
        { \
            subContext->Add<dataType>(this->Get<dataType>()); \
        }

        COPY_DATA_IF_EXISTS(Data::InstallerDownloadAuthenticators);
    }

    void Context::EnableSignalTerminationHandler(bool enabled)
    {
        ShutdownMonitoring::TerminationSignalHandler::EnableListener(enabled, this);
        m_disableSignalTerminationHandlerOnExit = enabled;
    }

    void Context::UpdateForArgs()
    {
        // Change logging level to Info if Verbose not requested
        if (Args.Contains(Args::Type::VerboseLogs))
        {
            Logging::Log().SetLevel(Logging::Level::Verbose);
        }

        // Disable warnings if requested
        if (Args.Contains(Args::Type::IgnoreWarnings))
        {
            Reporter.SetLevelMask(Reporter::Level::Warning, false);
        }

        // Set proxy
        if (Args.Contains(Args::Type::Proxy))
        {
            Network().SetProxyUri(std::string{ Args.GetArg(Args::Type::Proxy) });
        }
        else if (Args.Contains(Args::Type::NoProxy))
        {
            Network().SetProxyUri(std::nullopt);
        }

        // Set visual style
        if (Args.Contains(Args::Type::NoVT))
        {
            Reporter.SetStyle(VisualStyle::NoVT);
        }
        else if (Args.Contains(Args::Type::RetroStyle))
        {
            Reporter.SetStyle(VisualStyle::Retro);
        }
        else if (Args.Contains(Args::Type::RainbowStyle))
        {
            Reporter.SetStyle(VisualStyle::Rainbow);
        }
        else
        {
            Reporter.SetStyle(User().Get<Setting::ProgressBarVisualStyle>());
        }
    }

    void Context::Terminate(HRESULT hr, std::string_view file, size_t line)
    {
        if (hr == APPINSTALLER_CLI_ERROR_CTRL_SIGNAL_RECEIVED)
        {
            ++m_CtrlSignalCount;
            // Use a more recognizable error
            hr = E_ABORT;

            // If things aren't terminating fast enough for the user, they will probably press CTRL+C again.
            // In that case, we should forcibly terminate.
            // Unless we want to spin a separate thread for all work, we have to just exit here.
            if (m_CtrlSignalCount >= 2)
            {
                Reporter.CloseOutputStream(true);
                Logging::Telemetry().LogCommandTermination(hr, file, line);
                std::exit(hr);
            }
        }
        else if (hr == APPINSTALLER_CLI_ERROR_APPTERMINATION_RECEIVED)
        {
            AICLI_LOG(CLI, Info, << "Got app termination signal");
            hr = E_ABORT;
        }

        Logging::Telemetry().LogCommandTermination(hr, file, line);

        if (!m_isTerminated)
        {
            SetTerminationHR(hr);
        }
    }

    void Context::SetTerminationHR(HRESULT hr)
    {
        m_terminationHR = hr;
        m_isTerminated = true;
    }

    void Context::Cancel(CancelReason reason, bool bypassUser)
    {
        HRESULT hr = ToHRESULT(reason);

        Terminate(hr);
        Reporter.CancelInProgressTask(bypassUser, reason);
    }

    void Context::SetExecutionStage(Workflow::ExecutionStage stage)
    {
        if (m_executionStage == stage)
        {
            return;
        }
        else if (m_executionStage > stage)
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), "Reporting ExecutionStage to an earlier Stage: current[%d], new[%d]", ToIntegral(m_executionStage), ToIntegral(stage));
        }

        m_executionStage = stage;
        GetThreadGlobals().GetTelemetryLogger().SetExecutionStage(static_cast<uint32_t>(m_executionStage));
    }

    AppInstaller::ThreadLocalStorage::WingetThreadGlobals& Context::GetThreadGlobals()
    {
        return *m_threadGlobals;
    }

    std::shared_ptr<ThreadLocalStorage::WingetThreadGlobals> Context::GetSharedThreadGlobals()
    {
        return m_threadGlobals;
    }

    std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> Context::SetForCurrentThread()
    {
        return m_threadGlobals->SetForCurrentThread();
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    bool Context::ShouldExecuteWorkflowTask(const Workflow::WorkflowTask& task)
    {
        return (m_shouldExecuteWorkflowTask ? m_shouldExecuteWorkflowTask(task) : true);
    }
#endif

    void ContextEnumBasedVariantMapActionCallback(const void* map, Data data, EnumBasedVariantMapAction action)
    {
        switch (action)
        {
        case EnumBasedVariantMapAction::Add:
            AICLI_LOG(Workflow, Verbose, << "Setting data item: " << data);
            break;
        case EnumBasedVariantMapAction::Contains:
            AICLI_LOG(Workflow, Verbose, << "Checking data item: " << data);
            break;
        case EnumBasedVariantMapAction::Get:
            AICLI_LOG(Workflow, Verbose, << "Getting data item: " << data);
            break;
        }

        UNREFERENCED_PARAMETER(map);
    }

    std::string Context::GetResumeId()
    {
        return m_checkpointManager->GetResumeId();
    }

    std::optional<Checkpoint<AutomaticCheckpointData>> Context::LoadCheckpoint(const std::string& resumeId)
    {
        m_checkpointManager = std::make_unique<AppInstaller::Checkpoints::CheckpointManager>(resumeId);
        return m_checkpointManager->GetAutomaticCheckpoint();
    }

    std::vector<AppInstaller::Checkpoints::Checkpoint<Execution::Data>> Context::GetCheckpoints()
    {
        return m_checkpointManager->GetCheckpoints();
    }

    void Context::Checkpoint(std::string_view checkpointName, std::vector<Execution::Data> contextData)
    {
        UNREFERENCED_PARAMETER(checkpointName);
        UNREFERENCED_PARAMETER(contextData);

        if (!m_checkpointManager)
        {
            m_checkpointManager = std::make_unique<AppInstaller::Checkpoints::CheckpointManager>();
            m_checkpointManager->CreateAutomaticCheckpoint(*this);

            // Register for restart only when we first call checkpoint to support restarting from an unexpected shutdown.
            AppInstaller::Reboot::RegisterRestartForWER("resume -g " + GetResumeId());
        }

        // TODO: Capture context data for checkpoint.
    }
}
