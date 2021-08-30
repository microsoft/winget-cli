#include "pch.h"
#include "Public/winget/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    using namespace AppInstaller::Logging;

    DiagnosticLogger& ThreadGlobals::GetDiagnosticLogger()
    {
        return *(m_pDiagnosticLogger);
    }

    TelemetryTraceLogger& ThreadGlobals::GetTelemetryLogger()
    {
        return *(m_pTelemetryLogger);
    }

    PreviousThreadGlobals* ThreadGlobals::SetForCurrentThread()
    {
        Initialize();

        std::unique_ptr<PreviousThreadGlobals> p_prevThreadGlobals = std::make_unique<PreviousThreadGlobals>(SetOrGetThreadGlobals(true, this));

        return p_prevThreadGlobals.release();
    }

    void ThreadGlobals::Initialize()
    {
        try
        {
            std::call_once(loggerInitOnceFlag, [this]()
                {
                    m_pDiagnosticLogger = std::make_unique<DiagnosticLogger>();
                    m_pTelemetryLogger = std::make_unique<TelemetryTraceLogger>();

                    if (m_pTelemetryLogger)
                    {
                        m_pTelemetryLogger->Initialize();
                    }
                });
        }
        catch (...)
        {
            // May throw std::system_error if any condition prevents calls to call_once from executing as specified
            // May throw std::bad_alloc or any exception thrown by the constructor of TelemetryTraceLogger
            // Loggers are best effort and shouldn't block core functionality. So eat up the exceptions here
        }
    }

    ThreadGlobals* ThreadGlobals::GetForCurrentThread()
    {
        return SetOrGetThreadGlobals(false);
    }

    ThreadGlobals* ThreadGlobals::SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals)
    {
        thread_local AppInstaller::ThreadLocalStorage::ThreadGlobals* t_pThreadGlobals = nullptr;

        if (setThreadGlobals == true)
        {
            AppInstaller::ThreadLocalStorage::ThreadGlobals* previous_pThreadGlobals = t_pThreadGlobals;
            t_pThreadGlobals = pThreadGlobals;
            return previous_pThreadGlobals;
        }

        return t_pThreadGlobals;
    }

    PreviousThreadGlobals::~PreviousThreadGlobals()
    {
        std::ignore = ThreadGlobals::SetOrGetThreadGlobals(true, m_previous);
    }
}
