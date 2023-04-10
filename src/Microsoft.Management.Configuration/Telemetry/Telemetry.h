// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLanguageUtilities.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <cguid.h>
#include <string>
#include <string_view>

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

        // Sets the current activity identifier.
        void SetActivityId(const guid& value);

        // Return address of m_activityId
        const GUID* GetActivityId() const;

        // Store the passed in name of the Caller for COM calls
        void SetCaller(std::string_view caller);

        // Logs information about running a configuration unit.
        // The caller is expected to only call this for failures from publicly available units.
        void LogConfigUnitRun(
            const guid& setIdentifier,
            const guid& unitIdentifier,
            std::string_view unitName,
            std::string_view moduleName,
            ConfigurationUnitIntent unitIntent,
            ConfigurationUnitIntent runIntent,
            std::string_view action,
            hresult result,
            ConfigurationUnitResultSource failurePoint,
            const winrt::Windows::Foundation::Collections::ValueSet& settings) const noexcept;

    protected:
        bool IsTelemetryEnabled() const noexcept;

        CopyConstructibleAtomic<bool> m_isRuntimeEnabled{ true };

        GUID m_activityId = GUID_NULL;
        std::string m_caller;
    };
}
