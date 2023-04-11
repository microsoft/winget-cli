// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Telemetry.h"
#include "TraceLogging.h"
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>

#define AICLI_TraceLoggingStringView(_sv_,_name_) TraceLoggingCountedUtf8String(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)
#define AICLI_TraceLoggingWStringView(_sv_,_name_) TraceLoggingCountedWideString(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)

#define AICLI_TraceLoggingWriteActivity(_eventName_,...) TraceLoggingWriteActivity(\
g_hTraceProvider,\
_eventName_,\
GetActivityId(),\
nullptr,\
TraceLoggingCountedUtf8String(m_version.c_str(),  static_cast<ULONG>(m_version.size()), "CodeVersion"),\
TraceLoggingCountedUtf8String(m_caller.c_str(),  static_cast<ULONG>(m_caller.size()), "Caller"),\
__VA_ARGS__)

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TelemetryTraceLogger::TelemetryTraceLogger()
    {
        std::ignore = CoCreateGuid(&m_activityId);
        // TODO: Get client version info
    }

    void TelemetryTraceLogger::SetActivityId(const guid& value)
    {
        m_activityId = value;
    }

    const GUID* TelemetryTraceLogger::GetActivityId() const
    {
        return &m_activityId;
    }

    bool TelemetryTraceLogger::EnableRuntime(bool value)
    {
        return m_isRuntimeEnabled.exchange(value);
    }

    void TelemetryTraceLogger::SetCaller(std::string_view caller)
    {
        m_caller = caller;
    }

    void TelemetryTraceLogger::LogConfigUnitRun(
        const guid& setIdentifier,
        const guid& unitIdentifier,
        hstring unitName,
        hstring moduleName,
        ConfigurationUnitIntent unitIntent,
        ConfigurationUnitIntent runIntent,
        std::string_view action,
        hresult result,
        ConfigurationUnitResultSource failurePoint,
        std::wstring_view settingNames) const noexcept try
    {
        if (IsTelemetryEnabled())
        {
            // TODO: Integrate enablement and activity id with processor properties
            AICLI_TraceLoggingWriteActivity(
                "ConfigUnitRun",
                TraceLoggingGuid(setIdentifier, "SetID"),
                TraceLoggingGuid(unitIdentifier, "UnitID"),
                AICLI_TraceLoggingWStringView(unitName, "UnitName"),
                AICLI_TraceLoggingWStringView(moduleName, "ModuleName"),
                TraceLoggingInt32(static_cast<int32_t>(unitIntent), "UnitIntent"),
                TraceLoggingInt32(static_cast<int32_t>(runIntent), "RunIntent"),
                AICLI_TraceLoggingStringView(action, "Action"),
                TraceLoggingHResult(result, "Result"),
                TraceLoggingInt32(static_cast<int32_t>(failurePoint), "FailurePoint"),
                AICLI_TraceLoggingWStringView(settingNames, "SettingProvided"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigUnitRunIfAppropriate(
        const guid& setIdentifier,
        const Configuration::ConfigurationUnit& unit,
        ConfigurationUnitIntent runIntent,
        std::string_view action,
        const ConfigurationUnitResultInformation& resultInformation) const noexcept try
    {
        // We only want to send telemetry for failures of publicly available units.
        if (SUCCEEDED(static_cast<int32_t>(resultInformation.ResultCode())))
        {
            return;
        }

        // TODO: Use details to determine if the configuration unit is public as well
        IConfigurationUnitProcessorDetails details = unit.Details();
        if (!details)
        {
            return;
        }

        const winrt::Windows::Foundation::Collections::ValueSet& settings = unit.Settings();
        std::vector<hstring> settingNames;

        for (const auto& setting : settings)
        {
            settingNames.emplace_back(setting.Key());
        }

        LogConfigUnitRun(setIdentifier, unit.InstanceIdentifier(), unit.UnitName(), details.ModuleName(), unit.Intent(), runIntent, action, resultInformation.ResultCode(), resultInformation.ResultSource(), settingNames);
    }
    CATCH_LOG();

    bool TelemetryTraceLogger::IsTelemetryEnabled() const noexcept
    {
        return g_IsTelemetryProviderEnabled && m_isRuntimeEnabled;
    }
}
