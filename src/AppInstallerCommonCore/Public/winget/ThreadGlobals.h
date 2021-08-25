#pragma once

#include <AppInstallerLogging.h>
#include <AppInstallerTelemetry.h>
#include <mutex>

namespace AppInstaller::ThreadLocalStorage
{
    struct ThreadGlobals
    {
        ThreadGlobals() = default;
        ~ThreadGlobals() = default;

        AppInstaller::Logging::DiagnosticLogger& GetDiagnosticLogger();

        AppInstaller::Logging::TelemetryTraceLogger& GetTelemetryLogger();

        // Set Globals for Current Thread
        void SetForCurrentThread(bool isNewContext = false);

        // Clear Globals
        void Clear();

        // Return Globals for Current Thread
        static ThreadGlobals* GetForCurrentThread();

    private:

        // Set and return Globals for Current Thread
        static ThreadGlobals* SetOrGetThreadGlobals(bool setThreadGlobals, ThreadGlobals* pThreadGlobals = nullptr);

        std::unique_ptr<AppInstaller::Logging::DiagnosticLogger> m_pDiagnosticLogger;
        std::unique_ptr<AppInstaller::Logging::TelemetryTraceLogger> m_pTelemetryLogger;
        std::once_flag diagLoggerInitOnceFlag;
        std::once_flag telLoggerInitOnceFlag;
    };
}

