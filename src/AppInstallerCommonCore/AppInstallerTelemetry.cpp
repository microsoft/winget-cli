// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerTelemetry.h"

#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Logging
{
    namespace
    {
        void __stdcall wilResultLoggingCallback(const wil::FailureInfo& info) noexcept
        {
            Telemetry().LogFailure(info);
        }
    }

    TelemetryTraceLogger::TelemetryTraceLogger()
    {
        RegisterTraceLogging();
    }

    TelemetryTraceLogger::~TelemetryTraceLogger()
    {
        UnRegisterTraceLogging();
    }

    TelemetryTraceLogger& TelemetryTraceLogger::GetInstance()
    {
        static TelemetryTraceLogger instance;
        return instance;
    }

    void TelemetryTraceLogger::LogFailure(const wil::FailureInfo& failure) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWrite(g_hTelemetryProvider,
                "FailureInfo",
                TraceLoggingHResult(failure.hr, "hr"),
                TraceLoggingWideString(failure.pszMessage, "message"),
                TraceLoggingString(failure.pszModule, "module"),
                TraceLoggingUInt32(failure.threadId, "threadId"),
                TraceLoggingUInt32(static_cast<uint32_t>(failure.type), "type"),
                TraceLoggingString(failure.pszFile, "file"),
                TraceLoggingUInt32(failure.uLineNumber, "line"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        // Also send failure to the log
        AICLI_LOG(Fail, Error, << [&]() {
                wchar_t message[2048];
                GetFailureLogString(message, ARRAYSIZE(message), failure);
                return Utility::ConvertToUTF8(message);
            }());
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }
}