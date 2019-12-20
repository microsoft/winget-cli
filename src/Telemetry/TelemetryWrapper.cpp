// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TelemetryWrapper.h"

#define PKGMGR_CLIENT_EVENT_INFO "Information"
#define PKGMGR_CLIENT_MESSAGE "Message"

namespace AppInstaller::CLI
{
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

    void TelemetryTraceLogger::LogMessage(std::wstring_view message)
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWrite(g_hTelemetryProvider,
                PKGMGR_CLIENT_EVENT_INFO,
                TraceLoggingCountedWideString(message.data(), static_cast<ULONG>(message.size()), PKGMGR_CLIENT_MESSAGE),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        //ToDo if required add logging to disk file here 
    }
}