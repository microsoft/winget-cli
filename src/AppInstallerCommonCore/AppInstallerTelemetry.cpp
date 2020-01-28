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
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
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
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        AICLI_LOG(CLI, Info, << "AppInstallerCLI, version [" << version << "], activity [" << *GetActivityId() << ']');
    }
 
    void TelemetryTraceLogger::LogCommand(std::string_view CommandName) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "CommandFound",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(CommandName.data(), CommandName.size(), "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        AICLI_LOG(CLI, Info, << "Leaf command to execute: " << CommandName);
    }

    void TelemetryTraceLogger::LogCommandSuccess(std::string_view CommandName) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "CommandSuccess",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(CommandName.data(), CommandName.size(), "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        AICLI_LOG(CLI, Info, << "Leaf command succeeded: " << CommandName);
    }

    void TelemetryTraceLogger::LogManifestFields(std::string ToolName, std::string ToolVersion) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "ManifestFields",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(ToolName.c_str(), ToolName.size(),"ToolName"),
                TraceLoggingCountedString(ToolVersion.c_str(), ToolVersion.size(), "ToolVersion"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        }

        AICLI_LOG(CLI, Info, << "AppInstallerCLI, ToolName [" << ToolName << "], ToolVersion [" << ToolVersion << ']');
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }
}