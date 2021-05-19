#include "pch.h"
#include "Public/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    DiagnosticLogger& ThreadGlobals::GetDiagnosticLogger()
    {
        if (!m_pDiagnosticLogger)
        {
            try
            {
                std::call_once(diagLoggerInitOnceFlag, [this]()
                {
                    m_pDiagnosticLogger = std::make_unique<DiagnosticLogger>();
                });
            }
            catch (std::exception& ex)
            {
                // May throw std::system_error if any condition prevents calls to call_once from executing as specified
                // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
                // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
                AICLI_LOG(Log, Error, << "Exception caught when creating DIAGNOSTIC Trace logger: " << ex.what() << '\n');
            }
        }
        return *(m_pDiagnosticLogger.get());
    }

    TelemetryTraceLogger& ThreadGlobals::GetTelemetryLogger()
    {
        return *(m_pTelemetryLogger.get());
    }

    void ThreadGlobals::SetForCurrentThread()
    {
        try
        {
            m_pTelemetryLogger = std::make_unique<TelemetryTraceLogger>();
        }
        catch (std::exception& ex)
        {
            // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
            // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
            AICLI_LOG(Log, Error, << "Exception caught when creating TELEMETRY Trace logger: " << ex.what() << '\n');
        }

        ActivateThreadGlobals(this);

        m_pTelemetryLogger->SetUserSettingsStatus();
    }

    ThreadGlobals* ThreadGlobals::GetForCurrentThread()
    {
        return ActivateThreadGlobals();
    }
}
