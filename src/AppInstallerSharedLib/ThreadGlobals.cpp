#include "pch.h"
#include "Public/winget/ThreadGlobals.h"

namespace AppInstaller::ThreadLocalStorage
{
    using namespace AppInstaller::Logging;

    // Set and return Globals for Current Thread
    static ThreadGlobals* SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals = nullptr);

    ThreadGlobals::ThreadGlobals(ThreadGlobals& parent, create_sub_thread_globals_t)
    {
        parent.Initialize();
        m_pDiagnosticLogger = parent.m_pDiagnosticLogger;
        m_pTelemetryLogger = parent.m_pTelemetryLogger->CreateSubTraceLogger();
        // Flip the initialization flag
        std::call_once(m_loggerInitOnceFlag, []() {});
    }

    DiagnosticLogger& ThreadGlobals::GetDiagnosticLogger()
    {
        return *(m_pDiagnosticLogger);
    }

    TelemetryTraceLogger& ThreadGlobals::GetTelemetryLogger()
    {
        return *(m_pTelemetryLogger);
    }

    std::unique_ptr<PreviousThreadGlobals> ThreadGlobals::SetForCurrentThread()
    {
        Initialize();

        std::unique_ptr<PreviousThreadGlobals> p_prevThreadGlobals = std::make_unique<PreviousThreadGlobals>(SetOrGetThreadGlobals(true, this));

        return p_prevThreadGlobals;
    }

    void ThreadGlobals::Initialize()
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

    ThreadGlobals* ThreadGlobals::GetForCurrentThread()
    {
        return SetOrGetThreadGlobals(false);
    }

    ThreadGlobals* SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals)
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
        std::ignore = SetOrGetThreadGlobals(true, m_previous);
    }
}
