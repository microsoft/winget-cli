// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <winrt/Windows.Foundation.Collections.h>
#include "ConfigurationUnitResultInformation.h"
#include "ConfigurationSet.h"
#include "TestConfigurationSetResult.h"
#include "ApplyConfigurationSetResult.h"

#include <cguid.h>
#include <string>
#include <string_view>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Provides the ability to write telemetry events.
    struct TelemetryTraceLogger
    {
        TelemetryTraceLogger();

        TelemetryTraceLogger(const TelemetryTraceLogger&) = default;
        TelemetryTraceLogger& operator=(const TelemetryTraceLogger&) = default;

        TelemetryTraceLogger(TelemetryTraceLogger&&) = default;
        TelemetryTraceLogger& operator=(TelemetryTraceLogger&&) = default;

        // Control whether this trace logger is enabled at runtime.
        // Returns the previous value.
        bool EnableRuntime(bool value);

        // Returns a value indicating whether the logger is enabled.
        bool IsEnabled() const;

        // Sets the current activity identifier.
        void SetActivityId(const guid& value);

        // Return address of m_activityId
        const GUID* GetActivityId() const;

        // Store the passed in name of the caller
        void SetCaller(std::string_view caller);

        // Get the current caller value
        std::string_view GetCaller() const;

        static constexpr std::string_view GetAction = "get";
        static constexpr std::string_view ApplyAction = "apply";
        static constexpr std::string_view TestAction = "test";
        static constexpr std::string_view ExportAction = "export";

        // Logs information about running a configuration unit.
        // The caller is expected to only call this for failures from publicly available units.
        void LogConfigUnitRun(
            const guid& setIdentifier,
            const guid& unitIdentifier,
            hstring unitName,
            hstring moduleName,
            ConfigurationUnitIntent unitIntent,
            ConfigurationUnitIntent runIntent,
            std::string_view action,
            hresult result,
            ConfigurationUnitResultSource failurePoint,
            std::wstring_view settingNames) const noexcept;

        // Logs information about running a configuration unit in the appropriate conditions.
        void LogConfigUnitRunIfAppropriate(
            const guid& setIdentifier,
            const Configuration::ConfigurationUnit& unit,
            ConfigurationUnitIntent runIntent,
            std::string_view action,
            const IConfigurationUnitResultInformation& resultInformation) const noexcept;

        // The summary information for a specific unit intent.
        struct ProcessingSummaryForIntent
        {
            ConfigurationUnitIntent Intent;
            uint32_t Count;
            uint32_t Run;
            uint32_t Failed;
        };

        // Logs a processing summary event for a configuration set.
        void LogConfigProcessingSummary(
            const guid& setIdentifier,
            std::string_view inputHash,
            ConfigurationUnitIntent runIntent,
            hresult result,
            ConfigurationUnitResultSource failurePoint,
            const ProcessingSummaryForIntent& assertSummary,
            const ProcessingSummaryForIntent& informSummary,
            const ProcessingSummaryForIntent& applySummary) const noexcept;

        // Logs a processing summary event for a configuration set test run.
        void LogConfigProcessingSummaryForTest(
            const ConfigurationSet& configurationSet,
            const TestConfigurationSetResult& result) const noexcept;

        // Logs a processing summary event for a configuration set test run exception.
        void LogConfigProcessingSummaryForTestException(
            const ConfigurationSet& configurationSet,
            hresult error,
            const TestConfigurationSetResult& result) const noexcept;

        // Logs a processing summary event for a configuration set apply run.
        void LogConfigProcessingSummaryForApply(
            const ConfigurationSet& configurationSet,
            const ApplyConfigurationSetResult& result) const noexcept;

        // Logs a processing summary event for a configuration set apply run exception.
        void LogConfigProcessingSummaryForApplyException(
            const ConfigurationSet& configurationSet,
            hresult error,
            const ApplyConfigurationSetResult& result) const noexcept;

    protected:
        bool IsTelemetryEnabled() const noexcept;

        CopyConstructibleAtomic<bool> m_isRuntimeEnabled{ true };

        GUID m_activityId = GUID_NULL;
        std::string m_version;
        std::string m_caller;
    };
}
