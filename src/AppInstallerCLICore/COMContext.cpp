// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMContext.h"

namespace AppInstaller
{
    static constexpr std::string_view s_comLogFileNamePrefix = "WPM"sv;

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

    void COMContext::FireCallbacks(::AppInstaller::ReportType reportType, uint64_t current, uint64_t maximum, ProgressType progressType, ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
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

    void COMContext::EndProgress(bool)
    {
        FireCallbacks(ReportType::EndProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::SetExecutionStage(CLI::Workflow::ExecutionStage executionStage, bool)
    {
        m_executionStage = executionStage;
        FireCallbacks(ReportType::ExecutionPhaseUpdate, 0, 0, ProgressType::None, m_executionStage);
        Logging::SetExecutionStage(static_cast<uint32_t>(m_executionStage));
    }

    void COMContext::SetGlobalLoggers()
    {
        COMContext::SetLoggers();
    }

    void COMContext::SetContextLoggers(const std::wstring_view TelemetryCorelationJson, const std::string& caller)
    {
        m_corelationData = TelemetryCorelationJson;

        SetThreadGlobalsActive(true);
        SetLoggers();
        Logging::Telemetry().SetTelemetryCorelationJson(TelemetryCorelationJson);
        Logging::Telemetry().SetCaller(caller);
        Logging::Telemetry().LogStartup(true);
    }

    std::wstring_view COMContext::GetCorelationJson()
    {
        return m_corelationData;
    }

    void COMContext::SetLoggers()
    {
        Logging::Log().SetLevel(Logging::Level::Info);
        Logging::Log().EnableChannel(Logging::Channel::All);

        // TODO: Log to file for COM API calls only when debugging in visual studio
        Logging::AddFileLogger(s_comLogFileNamePrefix);
        Logging::BeginLogFileCleanup();

        Logging::AddTraceLogger();

        Logging::EnableWilFailureTelemetry();
    }
}
