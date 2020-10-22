// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <wil/result_macros.h>

#include <string_view>
#include <vector>

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

        // Logs the invoked command termination.
        void LogCommandTermination(HRESULT hr, std::string_view file, size_t line) noexcept;

        // Logs the invoked command termination.
        void LogException(std::string_view commandName, std::string_view type, std::string_view message) noexcept;

        // Logs whether the manifest used in workflow is local
        void LogIsManifestLocal(bool isLocalManifest) noexcept;

        // Logs the Manifest fields.
        void LogManifestFields(std::string_view id, std::string_view name, std::string_view version) noexcept;

        // Logs when there is no matching App found for search
        void LogNoAppMatch() noexcept;

        // Logs when there is multiple matching Apps found for search
        void LogMultiAppMatch() noexcept;

        // Logs the name and Id of app found
        void LogAppFound(std::string_view name, std::string_view id) noexcept;

        // Logs the selected installer details
        void LogSelectedInstaller(int arch, std::string_view url, std::string_view installerType, std::string_view scope, std::string_view language) noexcept;

        // Logs details of a search request.
        void LogSearchRequest(
            std::string_view type,
            std::string_view query,
            std::string_view id,
            std::string_view name,
            std::string_view moniker,
            std::string_view tag,
            std::string_view command,
            size_t maximum,
            std::string_view request);

        // Logs the Search Result
        void LogSearchResultCount(uint64_t resultCount) noexcept;

        // Logs a mismatch between the expected and actual hash values.
        void LogInstallerHashMismatch(
            std::string_view id,
            std::string_view version,
            std::string_view channel,
            const std::vector<uint8_t>& expected,
            const std::vector<uint8_t>& actual,
            bool overrideHashMismatch);

        // Logs a faild installation attempt.
        void LogInstallerFailure(std::string_view id, std::string_view version, std::string_view channel, std::string_view type, uint32_t errorCode);

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

    // An RAII object to disable telemetry during its lifetime.
    // Primarily used by the complete command to prevent messy input from spamming us.
    struct DisableTelemetryScope
    {
        DisableTelemetryScope();

        DisableTelemetryScope(const DisableTelemetryScope&) = delete;
        DisableTelemetryScope& operator=(const DisableTelemetryScope&) = delete;

        DisableTelemetryScope(DisableTelemetryScope&&) = default;
        DisableTelemetryScope& operator=(DisableTelemetryScope&&) = default;

        ~DisableTelemetryScope();

    private:
        DestructionToken m_token;
    };

    // Sets an execution stage to be reported when failures occur.
    void SetExecutionStage(uint32_t stage);

    // An RAII object to log telemetry as sub execution.
    // Does not support nested sub execution.
    struct SubExecutionTelemetryScope
    {
        SubExecutionTelemetryScope();

        SubExecutionTelemetryScope(const SubExecutionTelemetryScope&) = delete;
        SubExecutionTelemetryScope& operator=(const SubExecutionTelemetryScope&) = delete;

        SubExecutionTelemetryScope(SubExecutionTelemetryScope&&) = default;
        SubExecutionTelemetryScope& operator=(SubExecutionTelemetryScope&&) = default;

        ~SubExecutionTelemetryScope();

    private:
        static std::atomic_uint32_t m_sessionId;
    };
}
