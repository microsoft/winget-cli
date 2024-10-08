// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <wil/result_macros.h>

#include <string_view>
#include <vector>
#include <cguid.h>

namespace AppInstaller::Settings
{
    struct UserSettings;
}

namespace AppInstaller::Logging
{
    enum class FailureTypeEnum : UINT32
    {
        None = 0x0,

        // Failure type from FailureInfo in result_macros.h
        ResultException = 0x1, // THROW_...
        ResultReturn = 0x2, // RETURN_..._LOG or RETURN_..._MSG
        ResultLog = 0x3, // LOG_...
        ResultFailFast = 0x4, // FAIL_FAST_...

        // Other failure types from LogException()
        Unknown = 0x10000,
        WinrtHResultError = 0x10001,
        ResourceOpen = 0x10002,
        StdException = 0x10003,

        // Command termination
        CommandTermination = 0x20000,
    };

    // Contains all fields logged through the TelemetryTraceLogger. Last write wins.
    // This will be used to report a summary event upon destruction of the TelemetryTraceLogger.
    struct TelemetrySummary
    {
        TelemetrySummary() = default;

        // Selectively copy member fields for copy constructor;
        TelemetrySummary(const TelemetrySummary& other);
        TelemetrySummary& operator=(const TelemetrySummary&) = default;

        TelemetrySummary(TelemetrySummary&&) = default;
        TelemetrySummary& operator=(TelemetrySummary&&) = default;

        // Log wil failure, exception, command termination
        HRESULT FailureHResult = S_OK;
        std::wstring FailureMessage;
        std::string FailureModule;
        UINT32 FailureThreadId = 0;
        FailureTypeEnum FailureType = FailureTypeEnum::None;
        std::string FailureFile;
        UINT32 FailureLine = 0;

        // LogStartup
        bool IsCOMCall = false;

        // LogCommand
        std::string Command;

        // LogCommandSuccess
        bool CommandSuccess = false;

        // LogIsManifestLocal
        bool IsManifestLocal = false;

        // LogManifestFields, LogAppFound
        std::string PackageIdentifier;
        std::string PackageName;
        std::string PackageVersion;
        std::string Channel;
        std::string SourceIdentifier;

        // LogSelectedInstaller
        INT32 InstallerArchitecture = -1;
        std::string InstallerUrl;
        std::string InstallerType;
        std::string InstallerScope;
        std::string InstallerLocale;

        // LogSearchRequest
        std::string SearchType;
        std::string SearchQuery;
        std::string SearchId;
        std::string SearchName;
        std::string SearchMoniker;
        std::string SearchTag;
        std::string SearchCommand;
        UINT64 SearchMaximum = 0;
        std::string SearchRequest;

        // LogSearchResultCount
        UINT64 SearchResultCount = 0;

        // LogInstallerHashMismatch
        std::vector<uint8_t> HashMismatchExpected;
        std::vector<uint8_t> HashMismatchActual;
        bool HashMismatchOverride = false;
        uint64_t HashMismatchActualSize = 0;
        std::string HashMismatchContentType;

        // LogInstallerFailure
        std::string InstallerExecutionType;
        UINT32 InstallerErrorCode = 0;

        // LogUninstallerFailure
        std::string UninstallerExecutionType;
        UINT32 UninstallerErrorCode = 0;

        // LogRepairFailure
        std::string RepairExecutionType;
        UINT32 RepairErrorCode = 0;

        // LogSuccessfulInstallARPChange
        UINT64 ChangesToARP = 0;
        UINT64 MatchesInARP = 0;
        UINT64 ChangesThatMatch = 0;
        UINT64 ARPLanguage = 0;
        std::string ARPName;
        std::string ARPVersion;
        std::string ARPPublisher;

        // LogNonFatalDOError
        std::string DOUrl;
        HRESULT DOHResult = S_OK;
    };

    // This type contains the registration lifetime of the telemetry trace logging provider.
    // Due to the nature of trace logging, specific methods should be added per desired trace.
    // As there should not be a significantly large number of individual telemetry events,
    // this should not become a burden.
    struct TelemetryTraceLogger
    {
        TelemetryTraceLogger(bool useSummary = true);

        ~TelemetryTraceLogger();

        TelemetryTraceLogger(const TelemetryTraceLogger&) = default;
        TelemetryTraceLogger& operator=(const TelemetryTraceLogger&) = default;

        TelemetryTraceLogger(TelemetryTraceLogger&&) = default;
        TelemetryTraceLogger& operator=(TelemetryTraceLogger&&) = default;

        // Control whether this trace logger is enabled at runtime.
        bool DisableRuntime();
        void EnableRuntime();

        // Return address of m_activityId
        const GUID* GetActivityId() const;

        // Return address of m_parentActivityId
        const GUID* GetParentActivityId() const;

        // Capture if UserSettings is enabled and set user profile path
        void Initialize();

        // Try to capture if UserSettings is enabled and set user profile path, returns whether the action is successfully completed.
        // There is a possible circular dependency with the user settings. When initializing the telemetry, we need to read the settings
        // to make sure it's not disabled, but a failure when reading the settings would trigger a telemetry event. We work around that
        // by avoiding initialization (and thus disabling telemetry) until we have successfully read the settings. Subsequent calls to
        // TryInitialize() would finish the initialization.
        bool TryInitialize();

        // Store the passed in name of the Caller for COM calls
        void SetCaller(const std::string& caller);

        // Store the passed in Telemetry Correlation Json for COM calls
        void SetTelemetryCorrelationJson(const std::wstring_view jsonStr_view) noexcept;

        void SetExecutionStage(uint32_t stage) noexcept;

        std::unique_ptr<TelemetryTraceLogger> CreateSubTraceLogger() const;

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
        void LogException(FailureTypeEnum type, std::string_view message) const noexcept;

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
            bool overrideHashMismatch,
            uint64_t downloadSizeInBytes,
            const std::optional<std::string>& contentType) const noexcept;

        // Logs a failed installation attempt.
        void LogInstallerFailure(std::string_view id, std::string_view version, std::string_view channel, std::string_view type, uint32_t errorCode) const noexcept;

        // Logs a failed uninstallation attempt.
        void LogUninstallerFailure(std::string_view id, std::string_view version, std::string_view type, uint32_t errorCode) const noexcept;

        // Logs a failed repair attempt.
        void LogRepairFailure(std::string_view id, std::string_view version, std::string_view type, uint32_t errorCode) const noexcept;

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
        bool IsTelemetryEnabled() const noexcept;

        // Initializes flags that determine whether telemetry is enabled.
        void InitializeInternal(const AppInstaller::Settings::UserSettings& userSettings);

        // Used to anonymize a string to the best of our ability.
        // Should primarily be used on failure messages or paths if needed.
        std::wstring AnonymizeString(const wchar_t* input) const noexcept;
        std::wstring AnonymizeString(std::wstring_view input) const noexcept;

        // Flags used to determine whether to send telemetry. All of them are set during initialization and
        // are CopyConstructibleAtomic to minimize the impact of multiple simultaneous initialization attempts.
        // m_isSettingEnabled starts as false so we can don't send telemetry until we have read the
        // settings and confirmed that it is enabled.
        CopyConstructibleAtomic<bool> m_isSettingEnabled{ false };

        // We may decide to disable telemetry at runtime, for example, for command line completion.
        CopyConstructibleAtomic<bool> m_isRuntimeEnabled{ true };

        // We wait for initialization of the other flags before sending any events.
        CopyConstructibleAtomic<bool> m_isInitialized{ false };

        CopyConstructibleAtomic<uint32_t> m_executionStage{ 0 };

        GUID m_activityId = GUID_NULL;
        GUID m_parentActivityId = GUID_NULL;
        std::wstring m_telemetryCorrelationJsonW = L"{}";
        std::string m_caller;

        bool m_useSummary = true;
        mutable TelemetrySummary m_summary;

        // TODO: This and all related code could be removed after transition to summary event in back end.
        uint32_t m_subExecutionId;
    };

    // Helper to make the call sites look clean.
    TelemetryTraceLogger& Telemetry();

    // Turns on wil failure telemetry and logging.
    void EnableWilFailureTelemetry();

    // TODO: Temporary code to keep existing telemetry behavior for command execution cases.
    void UseGlobalTelemetryLoggerActivityIdOnly();

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
}
