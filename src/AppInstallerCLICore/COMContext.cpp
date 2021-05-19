// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "COMContext.h"

namespace AppInstaller
{

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
    }

    void COMContext::SetLoggers(std::string telemetryCorelationJson, std::string comCaller)
    {
        SetThreadGlobalsActive();

        Logging::Log().SetLevel(Logging::Level::Verbose);
        Logging::AddTraceLogger();

#ifdef _DEBUG
        // Log to file for COM API calls only when debugging in visual studio
        Logging::AddFileLogger();
        Logging::BeginLogFileCleanup();
#endif

        Logging::EnableWilFailureTelemetry();
        Logging::Telemetry().SetTelemetryCorelationJson(telemetryCorelationJson);
        Logging::Telemetry().SetCOMCaller(comCaller);
        Logging::Telemetry().LogStartup(true);
    }
}
