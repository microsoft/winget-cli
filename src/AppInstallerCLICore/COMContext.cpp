// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMContext.h"
#include <AppInstallerFileLogger.h>
#include <winget/TraceLogger.h>
#include <winget/OutputDebugStringLogger.h>

namespace AppInstaller::CLI::Execution
{
    static constexpr std::string_view s_comLogFileNamePrefix = "WinGetCOM"sv;

    NullStream::NullStream()
    {
        m_nullOut.reset(new std::ostream(&m_nullStreamBuf));
        m_nullIn.reset(new std::istream(&m_nullStreamBuf));
    }

    void COMContext::AddProgressCallbackFunction(ProgressCallBackFunction&& f)
    {
        std::lock_guard<std::mutex> lock{ m_callbackLock };
        m_comProgressCallbacks.push_back(std::move(f));
    }

    void COMContext::FireCallbacks(ReportType reportType, uint64_t current, uint64_t maximum, ProgressType progressType, ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
    {
        // Lock around iterating through the list. Callbacks should not do long running tasks.
        std::lock_guard<std::mutex> lock{ m_callbackLock };
        for (auto& callback : m_comProgressCallbacks)
        {
            callback(reportType, current, maximum, progressType, executionPhase);
        }
    };

    void COMContext::BeginProgress()
    {
        FireCallbacks(ReportType::BeginProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::OnProgress(uint64_t current, uint64_t maximum, ProgressType progressType)
    {
        FireCallbacks(ReportType::Progressing, current, maximum, progressType, m_executionStage);
    }

    void COMContext::SetProgressMessage(std::string_view)
    {
        // TODO: Consider sending message to COM progress
    }

    void COMContext::EndProgress(bool)
    {
        FireCallbacks(ReportType::EndProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::SetExecutionStage(CLI::Workflow::ExecutionStage executionStage)
    {
        m_executionStage = executionStage;
        FireCallbacks(ReportType::ExecutionPhaseUpdate, 0, 0, ProgressType::None, m_executionStage);
        GetThreadGlobals().GetTelemetryLogger().SetExecutionStage(static_cast<uint32_t>(m_executionStage));
    }

    void COMContext::SetContextLoggers(const std::wstring_view telemetryCorrelationJson, const std::string& caller)
    {
        m_correlationData = telemetryCorrelationJson;

        std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> setThreadGlobalsToPreviousState = this->SetForCurrentThread();

        SetLoggers();
        GetThreadGlobals().GetTelemetryLogger().SetTelemetryCorrelationJson(telemetryCorrelationJson);
        GetThreadGlobals().GetTelemetryLogger().SetCaller(caller);
        GetThreadGlobals().GetTelemetryLogger().LogStartup(true);
    }

    std::wstring_view COMContext::GetCorrelationJson()
    {
        return m_correlationData;
    }

    void COMContext::SetLoggers(std::optional<AppInstaller::Logging::Channel> channel, std::optional<AppInstaller::Logging::Level> level)
    {
        // Set up debug string logging during initialization
        Logging::OutputDebugStringLogger::Add();
        Logging::Log().EnableChannel(Logging::Channel::All);
        Logging::Log().SetLevel(Logging::Level::Verbose);

        Logging::Log().EnableChannel(channel.has_value() ? channel.value() : Settings::User().Get<Settings::Setting::LoggingChannelPreference>());
        Logging::Log().SetLevel(level.has_value() ? level.value() : Settings::User().Get<Settings::Setting::LoggingLevelPreference>());

        // TODO: Log to file for COM API calls only when debugging in visual studio
        Logging::FileLogger::Add(s_comLogFileNamePrefix);

        Logging::OutputDebugStringLogger::Remove();

#ifndef AICLI_DISABLE_TEST_HOOKS
        if (!Settings::User().Get<Settings::Setting::KeepAllLogFiles>())
#endif
        {
            // Initiate the background cleanup of the log file location.
            Logging::FileLogger::BeginCleanup();
        }

        Logging::TraceLogger::Add();

        Logging::EnableWilFailureTelemetry();
    }
}
