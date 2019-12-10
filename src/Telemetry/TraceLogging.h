// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WinEventLogLevels.h"
#include <TraceLoggingProvider.h>

#include "MicrosoftTelemetry.h"

// Keywords
#define KEYWORD_REPEATER         0x0000000000000001
#define KEYWORD_SCROLLER         0x0000000000000002
#define KEYWORD_PTR              0x0000000000000004
#define KEYWORD_SCROLLVIEWER     0x0000000000000008
#define KEYWORD_SWIPECONTROL     0x0000000000000010
#define KEYWORD_COMMANDBARFLYOUT 0x0000000000000020

// Common output formats
#define TRACE_MSG_METH L"%s[0x%p]()\n"
#define TRACE_MSG_METH_DBL L"%s[0x%p](%lf)\n"
#define TRACE_MSG_METH_DBL_DBL L"%s[0x%p](%lf, %lf)\n"
#define TRACE_MSG_METH_DBL_INT L"%s[0x%p](%lf, %d)\n"
#define TRACE_MSG_METH_DBL_DBL_INT L"%s[0x%p](%lf, %lf, %d)\n"
#define TRACE_MSG_METH_DBL_DBL_FLT L"%s[0x%p](%lf, %lf, %f)\n"
#define TRACE_MSG_METH_DBL_DBL_STR L"%s[0x%p](%lf, %lf, %s)\n"
#define TRACE_MSG_METH_FLT L"%s[0x%p](%f)\n"
#define TRACE_MSG_METH_FLT_FLT L"%s[0x%p](%f, %f)\n"
#define TRACE_MSG_METH_FLT_FLT_FLT L"%s[0x%p](%f, %f, %f)\n"
#define TRACE_MSG_METH_FLT_FLT_FLT_FLT L"%s[0x%p](%f, %f, %f, %f)\n"
#define TRACE_MSG_METH_FLT_FLT_STR_INT L"%s[0x%p](%f, %f, %s, %d)\n"
#define TRACE_MSG_METH_INT L"%s[0x%p](%d)\n"
#define TRACE_MSG_METH_INT_INT L"%s[0x%p](%d, %d)\n"
#define TRACE_MSG_METH_PTR L"%s[0x%p](0x%p)\n"
#define TRACE_MSG_METH_PTR_PTR L"%s[0x%p](0x%p, 0x%p)\n"
#define TRACE_MSG_METH_PTR_DBL L"%s[0x%p](0x%p, %lf)\n"
#define TRACE_MSG_METH_PTR_INT L"%s[0x%p](0x%p, %d)\n"
#define TRACE_MSG_METH_PTR_STR L"%s[0x%p](0x%p, %s)\n"
#define TRACE_MSG_METH_STR L"%s[0x%p](%s)\n"
#define TRACE_MSG_METH_STR_STR L"%s[0x%p](%s, %s)\n"
#define TRACE_MSG_METH_STR_DBL L"%s[0x%p](%s, %lf)\n"
#define TRACE_MSG_METH_STR_FLT L"%s[0x%p](%s, %f)\n"
#define TRACE_MSG_METH_STR_INT L"%s[0x%p](%s, %d)\n"
#define TRACE_MSG_METH_STR_STR_STR L"%s[0x%p](%s, %s, %s)\n"
#define TRACE_MSG_METH_STR_INT_INT L"%s[0x%p](%s, %d, %d)\n"
#define TRACE_MSG_METH_STR_FLT_FLT L"%s[0x%p](%s, %f, %f)\n"
#define TRACE_MSG_METH_STR_STR_FLT L"%s[0x%p](%s, %s, %f)\n"
#define TRACE_MSG_METH_STR_STR_INT_INT L"%s[0x%p](%s, %s, %d, %d)\n"

#define TRACE_MSG_METH_METH L"%s[0x%p] - calls %s()\n"
#define TRACE_MSG_METH_METH_INT L"%s[0x%p] - calls %s(%d)\n"
#define TRACE_MSG_METH_METH_STR L"%s[0x%p] - calls %s(%s)\n"
#define TRACE_MSG_METH_METH_STR_STR L"%s[0x%p] - calls %s(%s, %s)\n"
#define TRACE_MSG_METH_METH_FLT_STR L"%s[0x%p] - calls %s(%f, %s)\n"
#define TRACE_MSG_METH_METH_FLT_FLT_FLT L"%s[0x%p] - calls %s(%f, %f, %f)\n"

// Current method name
#define METH_NAME StringUtil::Utf8ToUtf16(__FUNCTION__).c_str()

// TraceLogging provider name for telemetry.
#define TELEMETRY_PROVIDER_NAME "Microsoft.PackageManager.Client"

TRACELOGGING_DECLARE_PROVIDER(g_hTelemetryProvider);
extern bool g_IsTelemetryProviderEnabled;
extern UCHAR g_TelemetryProviderLevel;
extern ULONGLONG g_TelemetryProviderMatchAnyKeyword;
extern GUID g_TelemetryProviderActivityId;

extern void RegisterTraceLogging();
extern void UnRegisterTraceLogging();
