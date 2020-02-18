// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerTelemetry.h"

#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

// Helper to print a GUID
std::ostream& operator<<(std::ostream& out, const GUID& guid)
{
    wchar_t buffer[256];

    if (StringFromGUID2(guid, buffer, ARRAYSIZE(buffer)))
    {
        out << AppInstaller::Utility::ConvertToUTF8(buffer);
    }
    else
    {
        out << "error";
    }

    return out;
}

namespace AppInstaller::Logging
{
    namespace
    {
        void __stdcall wilResultLoggingCallback(const wil::FailureInfo& info) noexcept
        {
            Telemetry().LogFailure(info);
        }

        GUID CreateGuid()
        {
            GUID result{};
            (void)CoCreateGuid(&result);
            return result;
        }

        const GUID* GetActivityId()
        {
            static GUID activityId = CreateGuid();
            return &activityId;
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
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "FailureInfo",
                GetActivityId(),
                nullptr,
                TraceLoggingHResult(failure.hr, "hr"),
                TraceLoggingWideString(failure.pszMessage, "message"),
                TraceLoggingString(failure.pszModule, "module"),
                TraceLoggingUInt32(failure.threadId, "threadId"),
                TraceLoggingUInt32(static_cast<uint32_t>(failure.type), "type"),
                TraceLoggingString(failure.pszFile, "file"),
                TraceLoggingUInt32(failure.uLineNumber, "line"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        // Also send failure to the log
        AICLI_LOG(Fail, Error, << [&]() {
                wchar_t message[2048];
                GetFailureLogString(message, ARRAYSIZE(message), failure);
                return Utility::ConvertToUTF8(message);
            }());
    }

    void TelemetryTraceLogger::LogStartup() noexcept
    {
        std::string version = Runtime::GetClientVersion();

        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "ClientStartup",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(version.c_str(), static_cast<ULONG>(version.size()), "version"),
                TraceLoggingWideString(GetCommandLineW(), "commandlineargs"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance|PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "AppInstallerCLI, version [" << version << "], activity [" << *GetActivityId() << ']');
    }
 
    void TelemetryTraceLogger::LogCommand(std::string_view commandName) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "CommandFound",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(commandName.data(), static_cast<ULONG>(commandName.size()), "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Leaf command to execute: " << commandName);
    }

    void TelemetryTraceLogger::LogCommandSuccess(std::string_view commandName) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "CommandSuccess",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(commandName.data(), static_cast<ULONG>(commandName.size()), "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Leaf command succeeded: " << commandName);
    }

    void TelemetryTraceLogger::LogManifestFields(const std::string& name, const std::string& version) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "ManifestFields",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(name.c_str(), static_cast<ULONG>(name.size()),"Name"),
                TraceLoggingCountedString(version.c_str(), static_cast<ULONG>(version.size()), "Version"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance|PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "AppInstallerCLI, Name [" << name << "], Version [" << version << ']');
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }
}