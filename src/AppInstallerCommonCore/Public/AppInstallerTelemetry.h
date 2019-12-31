// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string_view>

namespace AppInstaller::Logging
{
    // This type contains the registration lifetime of the telemetry trace logging provider.
    // Due to the nature of trace logging, specific methods should be added per desired trace.
    // As there should not be a significantly large number of individual telemetry events,
    // this should not become a burden.
    struct TelemetryTraceLogger
    {
        ~TelemetryTraceLogger();

        TelemetryTraceLogger(const TelemetryTraceLogger&) = delete;
        TelemetryTraceLogger& operator=(const TelemetryTraceLogger&) = delete;

        TelemetryTraceLogger(TelemetryTraceLogger&&) = delete;
        TelemetryTraceLogger& operator=(TelemetryTraceLogger&&) = delete;

        // Gets the singleton instance of this type.
        static TelemetryTraceLogger& GetInstance();

        // TODO: Remove in favor of specific event methods.
        void LogMessage(std::wstring_view message);

    private:
        TelemetryTraceLogger();
    };

    // Helper to make the call sites look clean.
    inline TelemetryTraceLogger& Telemetry()
    {
        return TelemetryTraceLogger::GetInstance();
    }
}
