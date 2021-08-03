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

    void COMContext::BeginProgress()
    {
        m_comProgressCallback(ReportType::BeginProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::OnProgress(uint64_t current, uint64_t maximum, ProgressType progressType)
    {
        m_comProgressCallback(ReportType::Progressing, current, maximum, progressType, m_executionStage);
    }

    void COMContext::EndProgress(bool)
    {
        m_comProgressCallback(ReportType::EndProgress, 0, 0, ProgressType::None, m_executionStage);
    };

    void COMContext::SetExecutionStage(CLI::Workflow::ExecutionStage executionStage, bool)
    {
        m_executionStage = executionStage;
        m_comProgressCallback(ReportType::ExecutionPhaseUpdate, 0, 0, ProgressType::None, m_executionStage);
        Logging::SetExecutionStage(static_cast<uint32_t>(m_executionStage));
    }

    void COMContext::SetLoggerContext(const std::wstring_view telemetryCorelationJson, const std::string& caller)
    {
        Logging::Telemetry().SetTelemetryCorelationJson(telemetryCorelationJson);
        Logging::Telemetry().SetCaller(caller);
        Logging::Telemetry().LogStartup(true);
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
