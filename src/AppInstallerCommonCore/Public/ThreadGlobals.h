#pragma once

#include <AppInstallerLogging.h>
#include <AppInstallerTelemetry.h>
#include <mutex>

namespace AppInstaller::ThreadLocalStorage
{
    using namespace AppInstaller::Logging;

    struct ThreadGlobals
    {
        ThreadGlobals() = default;
        ~ThreadGlobals() = default;

        DiagnosticLogger& GetDiagnosticLogger();

        TelemetryTraceLogger& GetTelemetryLogger();

        // Set Globals for Current Thread
        void SetForCurrentThread();

        // Return Globals for Current Thread
        static ThreadGlobals* GetForCurrentThread();

    private:

        // Set and return Globals for Current Thread
        static ThreadGlobals* ActivateThreadGlobals(ThreadGlobals* pThreadGlobals = nullptr);

        std::unique_ptr<DiagnosticLogger> m_pDiagnosticLogger;
        std::unique_ptr<TelemetryTraceLogger> m_pTelemetryLogger;
        std::once_flag diagLoggerInitOnceFlag;
        std::once_flag telLoggerInitOnceFlag;
    };
}

