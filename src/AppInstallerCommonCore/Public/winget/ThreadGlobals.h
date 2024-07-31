// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <winget/SharedThreadGlobals.h>
#include <AppInstallerTelemetry.h>
#include <mutex>

namespace AppInstaller::ThreadLocalStorage
{
    struct WingetThreadGlobals : public ThreadGlobals
    {
        WingetThreadGlobals() = default;
        virtual ~WingetThreadGlobals() = default;

        // Request that a sub ThreadGlobals be constructed from the given parent.
        struct create_sub_thread_globals_t {};
        WingetThreadGlobals(WingetThreadGlobals& parent, create_sub_thread_globals_t);

        AppInstaller::Logging::DiagnosticLogger& GetDiagnosticLogger() override;

        void* GetTelemetryObject() override;

        AppInstaller::Logging::TelemetryTraceLogger& GetTelemetryLogger();

        // Set Globals for Current Thread
        // Return RAII object with its ownership to set the AppInstaller ThreadLocalStorage back to previous state
        std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> SetForCurrentThread() override;

    private:

        void Initialize();

        std::shared_ptr<AppInstaller::Logging::DiagnosticLogger> m_pDiagnosticLogger;
        std::unique_ptr<AppInstaller::Logging::TelemetryTraceLogger> m_pTelemetryLogger;
        std::once_flag m_loggerInitOnceFlag;
    };
}
