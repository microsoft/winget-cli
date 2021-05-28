// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TraceLogging.h"

// GUID for Microsoft.PackageManager.Client : {c0cf606f-569b-5c20-27d9-88a745fa2175}
TRACELOGGING_DEFINE_PROVIDER(
    g_hTraceProvider,
    "Microsoft.PackageManager.Client",
    (0xc0cf606f, 0x569b, 0x5c20, 0x27, 0xd9, 0x88, 0xa7, 0x45, 0xfa, 0x21, 0x75),
    TraceLoggingOptionMicrosoftTelemetry());

bool g_IsTelemetryProviderEnabled{};
UCHAR g_TelemetryProviderLevel{};
ULONGLONG g_TelemetryProviderMatchAnyKeyword{};

void WINAPI TelemetryProviderEnabledCallback(
    _In_      LPCGUID /*sourceId*/,
    _In_      ULONG isEnabled,
    _In_      UCHAR level,
    _In_      ULONGLONG matchAnyKeyword,
    _In_      ULONGLONG /*matchAllKeywords*/,
    _In_opt_  PEVENT_FILTER_DESCRIPTOR /*filterData*/,
    _In_opt_  PVOID /*callbackContext*/)
{
    g_IsTelemetryProviderEnabled = !!isEnabled;
    g_TelemetryProviderLevel = level;
    g_TelemetryProviderMatchAnyKeyword = matchAnyKeyword;
}

void RegisterTraceLogging()
{
    TraceLoggingRegisterEx(g_hTraceProvider, TelemetryProviderEnabledCallback, nullptr);
}

void UnRegisterTraceLogging()
{
    TraceLoggingUnregister(g_hTraceProvider);
}
