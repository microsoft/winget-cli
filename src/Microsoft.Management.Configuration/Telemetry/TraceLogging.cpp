// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TraceLogging.h"

// GUID for Microsoft.Management.Configuration : {9be929c4-3582-4629-aaa2-f427a5032b33}
TRACELOGGING_DEFINE_PROVIDER(
    g_hTraceProvider,
    "Microsoft.Management.Configuration",
    (0x9be929c4, 0x3582, 0x4629, 0xaa, 0xa2, 0xf4, 0x27, 0xa5, 0x03, 0x2b, 0x33),
    TraceLoggingOptionMicrosoftTelemetry());

bool g_IsTelemetryProviderEnabled{};
UCHAR g_TelemetryProviderLevel{};
ULONGLONG g_TelemetryProviderMatchAnyKeyword{};

struct TraceProvider
{
    TraceProvider();

    ~TraceProvider();
};

TraceProvider g_TraceProvider{};

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

TraceProvider::TraceProvider()
{
    TraceLoggingRegisterEx(g_hTraceProvider, TelemetryProviderEnabledCallback, nullptr);
}

TraceProvider::~TraceProvider()
{
    TraceLoggingUnregister(g_hTraceProvider);
}
