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

    void TelemetryTraceLogger::LogNoAppMatch() noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "NoAppMatch",
                GetActivityId(),
                nullptr,
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "No app found matching input criteria");
    }

    void TelemetryTraceLogger::LogMultiAppMatch() noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "MultiAppMatch",
                GetActivityId(),
                nullptr,
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Multiple apps found matching input criteria");
    }

    void TelemetryTraceLogger::LogAppFound(const std::string& name, const std::string& id) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "AppFound",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(name.c_str(), static_cast<ULONG>(name.size()), "AppName"),
                TraceLoggingCountedString(id.c_str(), static_cast<ULONG>(id.size()), "id"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Found one app. App id: " << id << " App name: " << name);
    }

    void TelemetryTraceLogger::LogSelectedInstaller(int arch, const std::string& url, const std::string& installerType, const std::string& scope, const std::string& language) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SelectedInstaller",
                GetActivityId(),
                nullptr,
                TraceLoggingInt32(arch, "Arch"),
                TraceLoggingCountedString(url.c_str(), static_cast<ULONG>(url.size()), "URL"),
                TraceLoggingCountedString(installerType.c_str(), static_cast<ULONG>(installerType.size()), "InstallerType"),
                TraceLoggingCountedString(scope.c_str(), static_cast<ULONG>(scope.size()), "Scope"),
                TraceLoggingCountedString(language.c_str(), static_cast<ULONG>(language.size()), "Language"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Completed installer selection.");
        AICLI_LOG(CLI, Verbose, << "Selected installer arch: " << arch);
        AICLI_LOG(CLI, Verbose, << "Selected installer url: " << url);
        AICLI_LOG(CLI, Verbose, << "Selected installer InstallerType: " << installerType);
        AICLI_LOG(CLI, Verbose, << "Selected installer scope: " << scope);
        AICLI_LOG(CLI, Verbose, << "Selected installer language: " << language);

    }

    void TelemetryTraceLogger::LogSearchResult(const std::string& msg) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SearchResult",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(msg.c_str(), static_cast<ULONG>(msg.size()), "Msg"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }
}