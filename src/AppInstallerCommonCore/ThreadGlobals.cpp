// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    using namespace AppInstaller::Logging;

    WingetThreadGlobals::WingetThreadGlobals(WingetThreadGlobals& parent, create_sub_thread_globals_t)
    {
        parent.Initialize();
        m_pDiagnosticLogger = parent.m_pDiagnosticLogger;
        m_pTelemetryLogger = parent.m_pTelemetryLogger->CreateSubTraceLogger();
        // Flip the initialization flag
        std::call_once(m_loggerInitOnceFlag, []() {});
    }

    DiagnosticLogger& WingetThreadGlobals::GetDiagnosticLogger()
    {
        return *(m_pDiagnosticLogger);
    }

    void* WingetThreadGlobals::GetTelemetryObject()
    {
        return m_pTelemetryLogger.get();
    }

    TelemetryTraceLogger& WingetThreadGlobals::GetTelemetryLogger()
    {
        return *(m_pTelemetryLogger);
    }

    std::unique_ptr<PreviousThreadGlobals> WingetThreadGlobals::SetForCurrentThread()
    {
        Initialize();
        return ThreadGlobals::SetForCurrentThread();
    }

    void WingetThreadGlobals::Initialize()
    {
        try
        {
            std::call_once(m_loggerInitOnceFlag, [this]()
                {
                    m_pDiagnosticLogger = std::make_unique<DiagnosticLogger>();
                    m_pTelemetryLogger = std::make_unique<TelemetryTraceLogger>();

                    // The above make_unique for TelemetryTraceLogger will either create an object or will throw which is caught below.
                    m_pTelemetryLogger->Initialize();
                });
        }
        catch (...)
        {
            // May throw std::system_error if any condition prevents calls to call_once from executing as specified
            // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
            // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
        }
    }
}
