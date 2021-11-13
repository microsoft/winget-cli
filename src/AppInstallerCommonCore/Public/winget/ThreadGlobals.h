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

        // Constructor to create a ThreadGlobals with given diagnostic and telemetry loggers.
        // During SetForCurrentThread, no new diagnostic or telemetry loggers will be created.
        ThreadGlobals(
            std::shared_ptr<AppInstaller::Logging::DiagnosticLogger> diagnosticLogger,
            std::unique_ptr<AppInstaller::Logging::TelemetryTraceLogger>&& telemetryLogger);

        // Request that a sub ThreadGlobals be constructed from the given parent.
        struct create_sub_thread_globals_t {};
        ThreadGlobals(const ThreadGlobals& parent, create_sub_thread_globals_t);

        bool ContainsDiagnosticLogger() const;
        AppInstaller::Logging::DiagnosticLogger& GetDiagnosticLogger();

        bool ContainsTelemetryLogger() const;
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
