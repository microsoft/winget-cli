#pragma once

#include <AppInstallerLogging.h>
#include <AppInstallerTelemetry.h>
#include <mutex>

namespace AppInstaller::ThreadLocalStorage
{

    struct PreviousThreadGlobals;

    struct ThreadGlobals
    {
        ThreadGlobals() = default;
        ~ThreadGlobals() = default;

        AppInstaller::Logging::DiagnosticLogger& GetDiagnosticLogger();

        AppInstaller::Logging::TelemetryTraceLogger& GetTelemetryLogger();

        // Set Globals for Current Thread
        // Return RAII object with it's ownership to set the AppInstaller ThreadLocalStorage back to previous state
        AppInstaller::ThreadLocalStorage::PreviousThreadGlobals* SetForCurrentThread();

        // Return Globals for Current Thread
        static ThreadGlobals* GetForCurrentThread();

    private:
        
        void Initialize();

        // Set and return Globals for Current Thread
        static ThreadGlobals* SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals = nullptr);

        std::unique_ptr<AppInstaller::Logging::DiagnosticLogger> m_pDiagnosticLogger;
        std::unique_ptr<AppInstaller::Logging::TelemetryTraceLogger> m_pTelemetryLogger;
        std::once_flag loggerInitOnceFlag;
        
        friend struct PreviousThreadGlobals;
    };

    struct PreviousThreadGlobals
    {
        ~PreviousThreadGlobals();

        PreviousThreadGlobals(ThreadGlobals* previous) : m_previous(previous) {};

    private:

        ThreadGlobals* m_previous;
    };
}
