// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <wil/result_macros.h>

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

        // Logs the failure info.
        void LogFailure(const wil::FailureInfo& failure) noexcept;

        // Logs the initial process startup.
        void LogStartup() noexcept;

        // Logs the invoked command.
        void LogCommand(std::string_view commandName) noexcept;

        // Logs the invoked command success.
        void LogCommandSuccess(std::string_view commandName) noexcept;

        // Logs the Manifest fields.
        void LogManifestFields(const std::string& name, const std::string& version) noexcept;
         
        //Logs when there is no matching App found for search
        void LogNoAppMatch() noexcept;

        //Logs when there is multiple matching Apps found for search
        void LogMultiAppMatch() noexcept;

        //Logs the name and Id of app found
        void LogAppFound(const std::string& name, const std::string& id) noexcept;

        //Logs the selected installer details
        void LogSelectedInstaller(int arch, const std::string& url, const std::string& installerType, const std::string& scope, const std::string& language) noexcept;

        //Logs the Search Result
        void LogSearchResultCount(ULONG resultCount) noexcept;

    private:
        TelemetryTraceLogger();
    };

    // Helper to make the call sites look clean.
    inline TelemetryTraceLogger& Telemetry()
    {
        return TelemetryTraceLogger::GetInstance();
    }

    // Turns on wil failure telemetry and logging.
    void EnableWilFailureTelemetry();
}
