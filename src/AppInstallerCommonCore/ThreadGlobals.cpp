#include "pch.h"
#include "Public/winget/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    using namespace AppInstaller::Logging;

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
            catch (...)
            {
                // May throw std::system_error if any condition prevents calls to call_once from executing as specified
                // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
                // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
            }
        }
        return *(m_pDiagnosticLogger.get());
    }

    TelemetryTraceLogger& ThreadGlobals::GetTelemetryLogger()
    {
        return *(m_pTelemetryLogger.get());
    }

    void ThreadGlobals::Clear()
    {
        SetOrGetThreadGlobals(true, nullptr);
    }

    void ThreadGlobals::SetForCurrentThread(bool isNewContext)
    {
        try
        {
            if (isNewContext)
            {
                m_pTelemetryLogger = std::make_unique<TelemetryTraceLogger>();
            }
        }
        catch (...)
        {
            // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
            // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
        }

        SetOrGetThreadGlobals(true, this);

        if (isNewContext)
        {
            m_pTelemetryLogger->Initialize();
        }

    }

    ThreadGlobals* ThreadGlobals::GetForCurrentThread()
    {
        return SetOrGetThreadGlobals(false);
    }
}
