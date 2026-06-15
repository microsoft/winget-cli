// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigThreadGlobals.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    AppInstaller::Logging::DiagnosticLogger& ConfigThreadGlobals::GetDiagnosticLogger()
    {
        return m_logger;
    }

    void* ConfigThreadGlobals::GetTelemetryObject()
    {
        return &m_telemetry;
    }

    TelemetryTraceLogger& ConfigThreadGlobals::GetTelemetryLogger()
    {
        return m_telemetry;
    }

    const TelemetryTraceLogger& ConfigThreadGlobals::GetTelemetryLogger() const
    {
        return m_telemetry;
    }
}
