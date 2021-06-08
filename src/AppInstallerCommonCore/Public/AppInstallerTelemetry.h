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
        virtual ~TelemetryTraceLogger();

        TelemetryTraceLogger(const TelemetryTraceLogger&) = default;
        TelemetryTraceLogger& operator=(const TelemetryTraceLogger&) = default;

        TelemetryTraceLogger(TelemetryTraceLogger&&) = default;
        TelemetryTraceLogger& operator=(TelemetryTraceLogger&&) = default;

        // Gets the singleton instance of this type.
        static TelemetryTraceLogger& GetInstance();

        // Control whether this trace logger is enabled at runtime.
        bool DisableRuntime();
        void EnableRuntime();

        // Store the passed in name of the Caller for COM calls
        void SetCaller(const std::string& caller);

        // Store the passed in Telemetry Corelation Json for COM calls
        void SetTelemetryCorelationJson(const std::wstring_view jsonStr_view) noexcept;

        // Logs the failure info.
        void LogFailure(const wil::FailureInfo& failure) const noexcept;

        // Logs the initial process startup.
        void LogStartup(bool isCOMCall = false) const noexcept;

        // Logs the invoked command.
        void LogCommand(std::string_view commandName) const noexcept;

        // Logs the invoked command success.
        void LogCommandSuccess(std::string_view commandName) const noexcept;

        // Logs the invoked command termination.
        void LogCommandTermination(HRESULT hr, std::string_view file, size_t line) const noexcept;

        // Logs the invoked command termination.
        void LogException(std::string_view commandName, std::string_view type, std::string_view message) const noexcept;

        // Logs whether the manifest used in workflow is local
        void LogIsManifestLocal(bool isLocalManifest) const noexcept;

        // Logs the Manifest fields.
        void LogManifestFields(std::string_view id, std::string_view name, std::string_view version) const noexcept;

        // Logs when there is no matching App found for search
        void LogNoAppMatch() const noexcept;

        // Logs when there is multiple matching Apps found for search
        void LogMultiAppMatch() const noexcept;

        // Logs the name and Id of app found
        void LogAppFound(std::string_view name, std::string_view id) const noexcept;

        // Logs the selected installer details
        void LogSelectedInstaller(int arch, std::string_view url, std::string_view installerType, std::string_view scope, std::string_view language) const noexcept;

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
            std::string_view request) const noexcept;

        // Logs the Search Result
        void LogSearchResultCount(uint64_t resultCount) const noexcept;

        // Logs a mismatch between the expected and actual hash values.
        void LogInstallerHashMismatch(
            std::string_view id,
            std::string_view version,
            std::string_view channel,
            const std::vector<uint8_t>& expected,
            const std::vector<uint8_t>& actual,
            bool overrideHashMismatch) const noexcept;

        // Logs a failed installation attempt.
        void LogInstallerFailure(std::string_view id, std::string_view version, std::string_view channel, std::string_view type, uint32_t errorCode) const noexcept;

        // Logs a failed uninstallation attempt.
        void LogUninstallerFailure(std::string_view id, std::string_view version, std::string_view type, uint32_t errorCode) const noexcept;

        // Logs data about the changes that ocurred in the ARP entries based on an install.
        // First 4 arguments are well known values for the package that we installed.
        // The next 3 are counts of the number of packages in each category.
        // The last 4 are the fields directly from the ARP entry that has been determined to be related to the package that
        // was installed, or they will be empty if there is no data or ambiguity about which entry should be logged.
        virtual void LogSuccessfulInstallARPChange(
            std::string_view sourceIdentifier,
            std::string_view packageIdentifier,
            std::string_view packageVersion,
            std::string_view packageChannel,
            size_t changesToARP,
            size_t matchesInARP,
            size_t countOfIntersectionOfChangesAndMatches,
            std::string_view arpName,
            std::string_view arpVersion,
            std::string_view arpPublisher,
            std::string_view arpLanguage) const noexcept;

        void LogNonFatalDOError(std::string_view url, HRESULT hr) const noexcept;

    protected:
        TelemetryTraceLogger();

        bool IsTelemetryEnabled() const noexcept;

        // Used to anonymize a string to the best of our ability.
        // Should primarily be used on failure messages or paths if needed.
        std::wstring AnonymizeString(const wchar_t* input) const noexcept;
        std::wstring AnonymizeString(std::wstring_view input) const noexcept;

        bool m_isSettingEnabled = true;
        std::atomic_bool m_isRuntimeEnabled{ true };

        std::wstring m_telemetryCorelationJsonW = L"{}";
        std::string m_caller;

        // Data that is needed by AnonymizeString
        std::wstring m_userProfile;
    };

    // Helper to make the call sites look clean.
    TelemetryTraceLogger& Telemetry();

    // Turns on wil failure telemetry and logging.
    void EnableWilFailureTelemetry();

    const GUID* GetActivityId(bool isNewActivity);

    // Set ActivityId
    void SetActivityId();

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
