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

#ifdef AICLI_DISABLE_TEST_HOOKS

#define WinGet_EventItem(_value_,_name_) 
#define WinGet_SummaryForIntentItem(_forIntent_,_name_,_pluralName_) 
#define WinGet_WriteEventToDiagnostics(_eventName_,...) 

#else

struct WinGetAbsorbVA_ARGSCommas
{
    WinGetAbsorbVA_ARGSCommas(int, int) {}
};

inline std::ostream& operator<<(std::ostream& out, const WinGetAbsorbVA_ARGSCommas&) { return out; }
inline std::ostream& operator<<(std::ostream& out, std::wstring_view value) { out << AppInstaller::Utility::ConvertToUTF8(value); return out; }

#define WinGet_EventItem(_value_,_name_) \
    0) << (_name_) << ": " << (_value_) << '\n' << WinGetAbsorbVA_ARGSCommas(0

#define WinGet_SummaryForIntentItem(_forIntent_,_name_,_pluralName_) \
    WinGet_EventItem(_forIntent_.Count, _name_ ## "Count"), \
    WinGet_EventItem(_forIntent_.Run, _pluralName_ ## "Run"), \
    WinGet_EventItem(_forIntent_.Failed, _pluralName_ ## "Failed")

#define WinGet_WriteEventToDiagnostics(_eventName_,...) \
{ \
    std::ostringstream _debugEventStream; \
    _debugEventStream << \
        "#DebugEventStream\n" << \
        "Event: " << (_eventName_) << '\n' << \
        "ActivityID: " << *GetActivityId() << '\n' << \
        "CodeVersion: " << m_version << '\n' << \
        "Caller: " << m_caller << '\n' \
        << WinGetAbsorbVA_ARGSCommas(0, __VA_ARGS__ ,0) \
        ; \
    AICLI_LOG_LARGE_STRING(Config, Verbose, , _debugEventStream.str()); \
}

#endif

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        // The data collected from running through a set of results.
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

        void ProcessUnitResult(const Configuration::ConfigurationUnit unit, const IConfigurationUnitResultInformation& resultInformation, ConfigRunSummaryData& result)
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
            case ConfigurationUnitIntent::Unknown:
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

    std::string_view TelemetryTraceLogger::GetCaller() const
    {
        return m_caller;
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
        // Change unknown to Apply for telemetry, as it will have been treated that way
        if (unitIntent == ConfigurationUnitIntent::Unknown)
        {
            unitIntent = ConfigurationUnitIntent::Apply;
        }

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
                AICLI_TraceLoggingWStringView(settingNames, "SettingsProvided"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            // Keep in sync with above event!
            WinGet_WriteEventToDiagnostics(
                "ConfigUnitRun",
                WinGet_EventItem(setIdentifier, "SetID"),
                WinGet_EventItem(unitIdentifier, "UnitID"),
                WinGet_EventItem(unitName, "UnitName"),
                WinGet_EventItem(moduleName, "ModuleName"),
                WinGet_EventItem(static_cast<int32_t>(unitIntent), "UnitIntent"),
                WinGet_EventItem(static_cast<int32_t>(runIntent), "RunIntent"),
                WinGet_EventItem(action, "Action"),
                WinGet_EventItem(result, "Result"),
                WinGet_EventItem(static_cast<int32_t>(failurePoint), "FailurePoint"),
                WinGet_EventItem(settingNames, "SettingsProvided"));
        }
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigUnitRunIfAppropriate(
        const guid& setIdentifier,
        const Configuration::ConfigurationUnit& unit,
        ConfigurationUnitIntent runIntent,
        std::string_view action,
        const IConfigurationUnitResultInformation& resultInformation) const noexcept try
    {
        if (!IsTelemetryEnabled())
        {
            return;
        }

        // We only want to send telemetry for publicly available units.
        IConfigurationUnitProcessorDetails details = unit.Details();
        if (!details || !details.IsPublic())
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
        if (!allSettingsNames.empty())
        {
            allSettingsNames.pop_back();
        }

        LogConfigUnitRun(setIdentifier, unit.InstanceIdentifier(), unit.Type(), details.ModuleName(), unit.Intent(), runIntent, action, resultInformation.ResultCode(), resultInformation.ResultSource(), allSettingsNames);
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigProcessingSummary(
        const guid& setIdentifier,
        std::string_view inputHash,
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
                AICLI_TraceLoggingStringView(inputHash, "InputHash"),
                TraceLoggingBool(false, "FromHistory"), // deprecated
                TraceLoggingInt32(static_cast<int32_t>(runIntent), "RunIntent"),
                TraceLoggingHResult(result, "Result"),
                TraceLoggingInt32(static_cast<int32_t>(failurePoint), "FailurePoint"),
                AICLI_TraceLoggingProcessingSummaryForIntent(assertSummary, "Assert", "Asserts"),
                AICLI_TraceLoggingProcessingSummaryForIntent(informSummary, "Inform", "Informs"),
                AICLI_TraceLoggingProcessingSummaryForIntent(applySummary, "Apply", "Applies"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            // Keep in sync with above event!
            WinGet_WriteEventToDiagnostics(
                "ConfigProcessingSummary",
                WinGet_EventItem(setIdentifier, "SetID"),
                WinGet_EventItem(inputHash, "InputHash"),
                WinGet_EventItem(false, "FromHistory"), // deprecated
                WinGet_EventItem(static_cast<int32_t>(runIntent), "RunIntent"),
                WinGet_EventItem(result, "Result"),
                WinGet_EventItem(static_cast<int32_t>(failurePoint), "FailurePoint"),
                WinGet_SummaryForIntentItem(assertSummary, "Assert", "Asserts"),
                WinGet_SummaryForIntentItem(informSummary, "Inform", "Informs"),
                WinGet_SummaryForIntentItem(applySummary, "Apply", "Applies"));
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

        LogConfigProcessingSummary(configurationSet.InstanceIdentifier(), configurationSet.GetInputHash(), ConfigurationUnitIntent::Assert,
            summaryData.Result, summaryData.FailurePoint, summaryData.AssertSummary, summaryData.InformSummary, summaryData.ApplySummary);
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigProcessingSummaryForTestException(
        const ConfigurationSet& configurationSet,
        hresult error,
        const TestConfigurationSetResult& result) const noexcept try
    {
        if (!IsTelemetryEnabled())
        {
            return;
        }

        ConfigRunSummaryData summaryData = ProcessRunResult(result.UnitResults());

        LogConfigProcessingSummary(configurationSet.InstanceIdentifier(), configurationSet.GetInputHash(), ConfigurationUnitIntent::Assert,
            error, ConfigurationUnitResultSource::Internal, summaryData.AssertSummary, summaryData.InformSummary, summaryData.ApplySummary);
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

        LogConfigProcessingSummary(configurationSet.InstanceIdentifier(), configurationSet.GetInputHash(), ConfigurationUnitIntent::Apply,
            result.ResultCode(), summaryData.FailurePoint, summaryData.AssertSummary, summaryData.InformSummary, summaryData.ApplySummary);
    }
    CATCH_LOG();

    void TelemetryTraceLogger::LogConfigProcessingSummaryForApplyException(
        const ConfigurationSet& configurationSet,
        hresult error,
        const ApplyConfigurationSetResult& result) const noexcept try
    {
        if (!IsTelemetryEnabled())
        {
            return;
        }

        ConfigRunSummaryData summaryData = ProcessRunResult(result.UnitResults());

        LogConfigProcessingSummary(configurationSet.InstanceIdentifier(), configurationSet.GetInputHash(), ConfigurationUnitIntent::Apply,
            error, ConfigurationUnitResultSource::Internal, summaryData.AssertSummary, summaryData.InformSummary, summaryData.ApplySummary);
    }
    CATCH_LOG();

    bool TelemetryTraceLogger::IsTelemetryEnabled() const noexcept
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        return g_IsTelemetryProviderEnabled && m_isRuntimeEnabled;
#else
        // For testing, only use the local enable state.
        return m_isRuntimeEnabled;
#endif
    }
}
