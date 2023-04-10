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
TraceLoggingCountedUtf8String(m_caller.c_str(),  static_cast<ULONG>(m_caller.size()), "Caller"),\
__VA_ARGS__)

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TelemetryTraceLogger::TelemetryTraceLogger()
    {
        std::ignore = CoCreateGuid(&m_activityId);
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
        std::string_view unitName,
        std::string_view moduleName,
        ConfigurationUnitIntent unitIntent,
        ConfigurationUnitIntent runIntent,
        std::string_view action,
        hresult result,
        ConfigurationUnitResultSource failurePoint,
        const winrt::Windows::Foundation::Collections::ValueSet& settings) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "ConfigUnitRun",
                TraceLoggingHResult(failure.hr, "HResult"),
                AICLI_TraceLoggingWStringView(anonMessage, "Message"),
                TraceLoggingString(failure.pszModule, "Module"),
                TraceLoggingUInt32(failure.threadId, "ThreadId"),
                TraceLoggingUInt32(static_cast<uint32_t>(failure.type), "Type"),
                TraceLoggingString(failure.pszFile, "File"),
                TraceLoggingUInt32(failure.uLineNumber, "Line"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }
    }

    bool TelemetryTraceLogger::IsTelemetryEnabled() const noexcept
    {
        return g_IsTelemetryProviderEnabled && m_isRuntimeEnabled;
    }
}
