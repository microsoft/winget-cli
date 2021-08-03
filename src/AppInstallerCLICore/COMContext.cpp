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

    std::vector<ProgressCallBackFunction> COMContext::GetCallbacks()
    {
        std::lock_guard<std::mutex> lock{ m_callbackLock };
        std::vector<ProgressCallBackFunction> tempComProgressCallbacks(m_comProgressCallbacks);
        return tempComProgressCallbacks;
    }

    void COMContext::FireCallbacks(::AppInstaller::ReportType reportType, uint64_t current, uint64_t maximum, ProgressType progressType, ::AppInstaller::CLI::Workflow::ExecutionStage executionPhase)
    {
        // Get a copy of the list so that it can be iterated safely while other threads may be adding new callbacks.
        std::vector<ProgressCallBackFunction> tempComProgressCallbacks = GetCallbacks();
        for (auto& callback : tempComProgressCallbacks)
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
    }

    void COMContext::SetLoggerContext(const std::wstring_view telemetryCorrelationJson, const std::string& caller)
    {
        m_correlationData = telemetryCorrelationJson;
        Logging::Telemetry().SetTelemetryCorrelationJson(telemetryCorrelationJson);
        Logging::Telemetry().SetCaller(caller);
        Logging::Telemetry().LogStartup(true);
    }
    
    std::wstring_view COMContext::GetCorrelationJson()
    {
        return m_correlationData;
    }

    void COMContext::SetLoggers()
    {
        Logging::Log().SetLevel(Logging::Level::Verbose);
        Logging::Log().EnableChannel(Logging::Channel::All);

        // TODO: Log to file for COM API calls only when debugging in visual studio
        Logging::AddFileLogger(s_comLogFileNamePrefix);
        Logging::BeginLogFileCleanup();

        Logging::AddTraceLogger();

        Logging::EnableWilFailureTelemetry();
    }
}
