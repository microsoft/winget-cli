// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerTelemetry.h"

#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

#define AICLI_TraceLoggingStringView(_sv_,_name_) TraceLoggingCountedString(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)

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
                TraceLoggingCountedString(version.c_str(), static_cast<ULONG>(version.size()), "Version"),
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
                AICLI_TraceLoggingStringView(commandName, "Command"),
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
                AICLI_TraceLoggingStringView(commandName, "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Leaf command succeeded: " << commandName);
    }

    void TelemetryTraceLogger::LogManifestFields(std::string_view id, std::string_view name, std::string_view version) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "ManifestFields",
                GetActivityId(),
                nullptr,
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(name,"Name"),
                AICLI_TraceLoggingStringView(version, "Version"),
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

    void TelemetryTraceLogger::LogAppFound(std::string_view name, std::string_view id) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "AppFound",
                GetActivityId(),
                nullptr,
                AICLI_TraceLoggingStringView(name, "AppName"),
                AICLI_TraceLoggingStringView(id, "Id"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Found one app. App id: " << id << " App name: " << name);
    }

    void TelemetryTraceLogger::LogSelectedInstaller(int arch, std::string_view url, std::string_view installerType, std::string_view scope, std::string_view language) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SelectedInstaller",
                GetActivityId(),
                nullptr,
                TraceLoggingInt32(arch, "Arch"),
                AICLI_TraceLoggingStringView(url, "URL"),
                AICLI_TraceLoggingStringView(installerType, "InstallerType"),
                AICLI_TraceLoggingStringView(scope, "Scope"),
                AICLI_TraceLoggingStringView(language, "Language"),
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

    void TelemetryTraceLogger::LogSearchRequest(
        std::string_view query,
        std::string_view id,
        std::string_view name,
        std::string_view moniker,
        std::string_view tag,
        std::string_view command,
        size_t maximum,
        std::string_view request)
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SearchRequest",
                GetActivityId(),
                nullptr,
                AICLI_TraceLoggingStringView(query, "Query"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(name, "Name"),
                AICLI_TraceLoggingStringView(moniker, "Moniker"),
                AICLI_TraceLoggingStringView(tag, "Tag"),
                AICLI_TraceLoggingStringView(command, "Command"),
                TraceLoggingUInt64(static_cast<UINT64>(maximum), "Maximum"),
                AICLI_TraceLoggingStringView(request, "Request"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }
    }

    void TelemetryTraceLogger::LogSearchResultCount(uint64_t resultCount) noexcept
    {
        if (g_IsTelemetryProviderEnabled)
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SearchResultCount",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt64(resultCount, "ResultCount"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }
}