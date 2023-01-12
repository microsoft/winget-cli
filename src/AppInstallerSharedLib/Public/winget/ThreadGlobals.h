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

        // Request that a sub ThreadGlobals be constructed from the given parent.
        struct create_sub_thread_globals_t {};
        ThreadGlobals(ThreadGlobals& parent, create_sub_thread_globals_t);

        AppInstaller::Logging::DiagnosticLogger& GetDiagnosticLogger();

        AppInstaller::Logging::TelemetryTraceLogger& GetTelemetryLogger();

        // Set Globals for Current Thread
        // Return RAII object with it's ownership to set the AppInstaller ThreadLocalStorage back to previous state
        std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> SetForCurrentThread();

        // Return Globals for Current Thread
        static ThreadGlobals* GetForCurrentThread();

    private:

        void Initialize();

        std::shared_ptr<AppInstaller::Logging::DiagnosticLogger> m_pDiagnosticLogger;
        std::unique_ptr<AppInstaller::Logging::TelemetryTraceLogger> m_pTelemetryLogger;
        std::once_flag m_loggerInitOnceFlag;
    };

    struct PreviousThreadGlobals
    {
        ~PreviousThreadGlobals();

        PreviousThreadGlobals(ThreadGlobals* previous) : m_previous(previous) {};

    private:

        ThreadGlobals* m_previous;
    };
}
