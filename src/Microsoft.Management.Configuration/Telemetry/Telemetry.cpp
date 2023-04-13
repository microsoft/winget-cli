// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Telemetry.h"
#include "TraceLogging.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>
#include <winget/Runtime.h>

#define AICLI_TraceLoggingStringView(_sv_,_name_) TraceLoggingCountedUtf8String(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)
#define AICLI_TraceLoggingWStringView(_sv_,_name_) TraceLoggingCountedWideString(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)

#define AICLI_TraceLoggingProcessingSummaryForIntent(_forIntent_,_name_,_pluralName_) \
    TraceLoggingUInt32(_forIntent_.Count, _name_ ## "Count"), \
    TraceLoggingUInt32(_forIntent_.Run, _pluralName_ ## "Run"), \
    TraceLoggingUInt32(_forIntent_.Failed, _pluralName_ ## "Failed")

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
    namespace
    {
        // The data collected from running throught a set of results.
        struct ConfigRunSummaryData
        {
            hresult Result = S_OK;
            ConfigurationUnitResultSource FailurePoint = ConfigurationUnitResultSource::None;
            TelemetryTraceLogger::ProcessingSummaryForIntent AssertSummary{ ConfigurationUnitIntent::Assert };
            TelemetryTraceLogger::ProcessingSummaryForIntent InformSummary{ ConfigurationUnitIntent::Inform };
            TelemetryTraceLogger::ProcessingSummaryForIntent ApplySummary{ ConfigurationUnitIntent::Apply };
        };

        size_t GetPriority(ConfigurationUnitResultSource source)
        {
            switch (source)
            {
            case ConfigurationUnitResultSource::Internal: return 0;
            case ConfigurationUnitResultSource::UnitProcessing: return 100;
            case ConfigurationUnitResultSource::SystemState: return 200;
            case ConfigurationUnitResultSource::ConfigurationSet: return 300;
            case ConfigurationUnitResultSource::Precondition: return 400;
            default: return 500;
            case ConfigurationUnitResultSource::None: return 600;
            }
        }

        bool FirstHasPriority(ConfigurationUnitResultSource first, ConfigurationUnitResultSource second)
        {
            return GetPriority(first) < GetPriority(second);
        }

        void ProcessUnitResult(const Configuration::ConfigurationUnit unit, Configuration::ConfigurationUnitResultInformation resultInformation, ConfigRunSummaryData& result)
        {
            hresult resultCode = resultInformation.ResultCode();
            if (FAILED(resultCode))
            {
                if (result.Result == S_OK || result.Result == resultCode)
                {
                    result.Result = resultCode;
                }
                else
                {
                    result.Result = WINGET_CONFIG_ERROR_SET_APPLY_FAILED;
                }
            }

            ConfigurationUnitResultSource unitFailurePoint = resultInformation.ResultSource();
            if (FirstHasPriority(unitFailurePoint, result.FailurePoint))
            {
                result.FailurePoint = unitFailurePoint;
            }

            TelemetryTraceLogger::ProcessingSummaryForIntent* summaryItem = nullptr;
            switch (unit.Intent())
            {
            case ConfigurationUnitIntent::Assert:
                summaryItem = &result.AssertSummary;
                break;
            case ConfigurationUnitIntent::Inform:
                summaryItem = &result.InformSummary;
                break;
            case ConfigurationUnitIntent::Apply:
                summaryItem = &result.ApplySummary;
                break;
            default:
                return;
            }

            summaryItem->Count++;

            ConfigurationUnitResultSource resultSource = resultInformation.ResultSource();
            if (resultSource != ConfigurationUnitResultSource::Precondition &&
                resultSource != ConfigurationUnitResultSource::ConfigurationSet)
            {
                summaryItem->Run++;
            }

            if (FAILED(resultCode))
            {
                summaryItem->Failed++;
            }
        }

        // Runs through a set of results, summarizing them.
        template <typename Enumerable>
        ConfigRunSummaryData ProcessRunResult(const Enumerable& results)
        {
            ConfigRunSummaryData result;

            for (const auto& item : results)
            {
                ProcessUnitResult(item.Unit(), item.ResultInformation(), result);
            }

            return result;
        }
    }

    TelemetryTraceLogger::TelemetryTraceLogger()
    {
        std::ignore = CoCreateGuid(&m_activityId);
        m_version = AppInstaller::Runtime::GetClientVersion();
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

    bool TelemetryTraceLogger::IsEnabled() const
    {
        return m_isRuntimeEnabled;
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
        if (!IsTelemetryEnabled() || SUCCEEDED(static_cast<int32_t>(resultInformation.ResultCode())))
        {
            return;
        }

        // TODO: Use details to determine if the configuration unit is public as well
        IConfigurationUnitProcessorDetails details = unit.Details();
        if (!details)
        {
            return;
        }

        // Create a single string from the set of top level setting names, ex. "a|b|c".
        const winrt::Windows::Foundation::Collections::ValueSet& settings = unit.Settings();
        std::wostringstream strstr;

        for (const auto& setting : settings)
        {
            strstr << static_cast<std::wstring_view>(setting.Key()) << L'|';
        }
        std::wstring allSettingsNames = strstr.str();
        allSettingsNames.pop_back();

        LogConfigUnitRun(setIdentifier, unit.InstanceIdentifier(), unit.UnitName(), details.ModuleName(), unit.Intent(), runIntent, action, resultInformation.ResultCode(), resultInformation.ResultSource(), allSettingsNames);
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigProcessingSummary(
        const guid& setIdentifier,
        bool fromHistory,
        ConfigurationUnitIntent runIntent,
        hresult result,
        ConfigurationUnitResultSource failurePoint,
        const ProcessingSummaryForIntent& assertSummary,
        const ProcessingSummaryForIntent& informSummary,
        const ProcessingSummaryForIntent& applySummary) const noexcept try
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "ConfigProcessingSummary",
                TraceLoggingGuid(setIdentifier, "SetID"),
                TraceLoggingBool(fromHistory, "FromHistory"),
                TraceLoggingInt32(static_cast<int32_t>(runIntent), "RunIntent"),
                TraceLoggingHResult(result, "Result"),
                TraceLoggingInt32(static_cast<int32_t>(failurePoint), "FailurePoint"),
                AICLI_TraceLoggingProcessingSummaryForIntent(assertSummary, "assert", "asserts"),
                AICLI_TraceLoggingProcessingSummaryForIntent(informSummary, "inform", "informs"),
                AICLI_TraceLoggingProcessingSummaryForIntent(applySummary, "apply", "applies"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigProcessingSummaryForTest(
        const ConfigurationSet& configurationSet,
        const TestConfigurationSetResult& result) const noexcept try
    {
        if (!IsTelemetryEnabled())
        {
            return;
        }

        ConfigRunSummaryData summaryData = ProcessRunResult(result.UnitResults());

        LogConfigProcessingSummary(configurationSet.InstanceIdentifier(), configurationSet.IsFromHistory(), ConfigurationUnitIntent::Assert,
            summaryData.Result, summaryData.FailurePoint, summaryData.AssertSummary, summaryData.InformSummary, summaryData.ApplySummary);
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigProcessingSummaryForApply(
        const ConfigurationSet& configurationSet,
        const ApplyConfigurationSetResult& result) const noexcept try
    {
        if (!IsTelemetryEnabled())
        {
            return;
        }

        ConfigRunSummaryData summaryData = ProcessRunResult(result.UnitResults());

        LogConfigProcessingSummary(configurationSet.InstanceIdentifier(), configurationSet.IsFromHistory(), ConfigurationUnitIntent::Apply,
            result.ResultCode(), summaryData.FailurePoint, summaryData.AssertSummary, summaryData.InformSummary, summaryData.ApplySummary);
    }
    CATCH_LOG();

    bool TelemetryTraceLogger::IsTelemetryEnabled() const noexcept
    {
        return g_IsTelemetryProviderEnabled && m_isRuntimeEnabled;
    }
}
