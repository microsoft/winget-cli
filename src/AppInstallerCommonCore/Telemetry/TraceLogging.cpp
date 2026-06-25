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

TRACELOGGING_DEFINE_PROVIDER(
    g_hStoreCriticalDataProvider,
    "Microsoft.Store",
    (0x5F0B026E, 0xBCC1, 0x5001, 0x95, 0xD3, 0x65, 0xE1, 0x70, 0xA1, 0x1E, 0xFA),
    TraceLoggingOptionGroup(0x5ECB0BAC, 0xB930, 0x47F5, 0xA8, 0xA4, 0xE8, 0x25, 0x35, 0x29, 0xED, 0xB7));

bool g_IsStoreTelemetryProviderEnabled{};

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

void WINAPI StoreTelemetryProviderEnabledCallback(
    _In_      LPCGUID /*sourceId*/,
    _In_      ULONG isEnabled,
    _In_      UCHAR /*level*/,
    _In_      ULONGLONG /*matchAnyKeyword*/,
    _In_      ULONGLONG /*matchAllKeywords*/,
    _In_opt_  PEVENT_FILTER_DESCRIPTOR /*filterData*/,
    _In_opt_  PVOID /*callbackContext*/)
{
    g_IsStoreTelemetryProviderEnabled = !!isEnabled;
}

TraceProvider::TraceProvider()
{
    TraceLoggingRegisterEx(g_hTraceProvider, TelemetryProviderEnabledCallback, nullptr);
    TraceLoggingRegisterEx(g_hStoreCriticalDataProvider, StoreTelemetryProviderEnabledCallback, nullptr);
}

TraceProvider::~TraceProvider()
{
    TraceLoggingUnregister(g_hTraceProvider);
    TraceLoggingUnregister(g_hStoreCriticalDataProvider);
}
