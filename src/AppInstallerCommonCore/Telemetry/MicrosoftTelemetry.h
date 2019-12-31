/* ++

Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. See LICENSE in the project root for license information.

Module Name:

    MicrosoftTelemetry.h

Abstract:

    Macro definitions used by this project's TraceLogging ETW providers:

    - Configuration macros that select the ETW Provider Groups to be used by
      this project.
    - Constants for tags that are commonly used in Microsoft's
      TraceLogging-based ETW.

    Different versions of this file use different definitions for the
    TraceLoggingOption configuration macros. The definitions in this file are
    empty. As a result, providers using this configuration file will not join
    any ETW Provider Groups and will not be given any special treatment by
    group-sensitive ETW listeners.

Environment:

    User mode or kernel mode.

--*/

#pragma once

// Configuration macro for use in TRACELOGGING_DEFINE_PROVIDER. The definition
// in this file configures the provider as a normal (non-telemetry) provider.
#define TraceLoggingOptionMicrosoftTelemetry() \
    // Empty definition for TraceLoggingOptionMicrosoftTelemetry

// Configuration macro for use in TRACELOGGING_DEFINE_PROVIDER. The definition
// in this file configures the provider as a normal (non-telemetry) provider.
#define TraceLoggingOptionWindowsCoreTelemetry() \
    // Empty definition for TraceLoggingOptionWindowsCoreTelemetry

// Event privacy tags. Use the PDT macro values for the tag parameter, e.g.:
// TraceLoggingWrite(...,
//   TelemetryPrivacyDataTag(PDT_BrowsingHistory | PDT_ProductAndServiceUsage),
//   ...);
#define TelemetryPrivacyDataTag(tag) TraceLoggingUInt64((tag), "PartA_PrivTags")
#define PDT_BrowsingHistory                    0x0000000000000002u
#define PDT_DeviceConnectivityAndConfiguration 0x0000000000000800u
#define PDT_InkingTypingAndSpeechUtterance     0x0000000000020000u
#define PDT_ProductAndServicePerformance       0x0000000001000000u
#define PDT_ProductAndServiceUsage             0x0000000002000000u
#define PDT_SoftwareSetupAndInventory          0x0000000080000000u

// Event categories specified via keywords, e.g.:
// TraceLoggingWrite(...,
//     TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
//     ...);
#define MICROSOFT_KEYWORD_CRITICAL_DATA 0x0000800000000000 // Bit 47
#define MICROSOFT_KEYWORD_MEASURES      0x0000400000000000 // Bit 46
#define MICROSOFT_KEYWORD_TELEMETRY     0x0000200000000000 // Bit 45
#define MICROSOFT_KEYWORD_RESERVED_44   0x0000100000000000 // Bit 44 (reserved for future assignment)

// Event categories specified via event tags, e.g.:
// TraceLoggingWrite(...,
//     TraceLoggingEventTag(MICROSOFT_EVENTTAG_REALTIME_LATENCY),
//     ...);
#define MICROSOFT_EVENTTAG_DROP_USER_IDS            0x00008000
#define MICROSOFT_EVENTTAG_AGGREGATE                0x00010000
#define MICROSOFT_EVENTTAG_DROP_PII_EXCEPT_IP       0x00020000
#define MICROSOFT_EVENTTAG_COSTDEFERRED_LATENCY     0x00040000
#define MICROSOFT_EVENTTAG_CORE_DATA                0x00080000
#define MICROSOFT_EVENTTAG_INJECT_XTOKEN            0x00100000
#define MICROSOFT_EVENTTAG_REALTIME_LATENCY         0x00200000
#define MICROSOFT_EVENTTAG_NORMAL_LATENCY           0x00400000
#define MICROSOFT_EVENTTAG_CRITICAL_PERSISTENCE     0x00800000
#define MICROSOFT_EVENTTAG_NORMAL_PERSISTENCE       0x01000000
#define MICROSOFT_EVENTTAG_DROP_PII                 0x02000000
#define MICROSOFT_EVENTTAG_HASH_PII                 0x04000000
#define MICROSOFT_EVENTTAG_MARK_PII                 0x08000000

// Field categories specified via field tags, e.g.:
// TraceLoggingWrite(...,
//     TraceLoggingString(szUser, "UserName", "User's name", MICROSOFT_FIELDTAG_HASH_PII),
//     ...);
#define MICROSOFT_FIELDTAG_DROP_PII 0x04000000
#define MICROSOFT_FIELDTAG_HASH_PII 0x08000000
