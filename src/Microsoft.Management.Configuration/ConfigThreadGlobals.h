// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SharedThreadGlobals.h>
#include <Telemetry/Telemetry.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Interface for access to values that are stored on a per-thread object.
    struct ConfigThreadGlobals : public AppInstaller::ThreadLocalStorage::ThreadGlobals
    {
        ConfigThreadGlobals() = default;
        virtual ~ConfigThreadGlobals() = default;

        AppInstaller::Logging::DiagnosticLogger& GetDiagnosticLogger() override;

        void* GetTelemetryObject() override;

        TelemetryTraceLogger& GetTelemetryLogger();
        const TelemetryTraceLogger& GetTelemetryLogger() const;

    protected:
        AppInstaller::Logging::DiagnosticLogger m_logger;
        TelemetryTraceLogger m_telemetry;
    };
}
