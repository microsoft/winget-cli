// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerStrings.h"

#define AICLI_TraceLoggingStringView(_sv_,_name_) TraceLoggingCountedUtf8String(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)

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
    using namespace Utility;

    namespace
    {
        static const uint32_t s_RootExecutionId = 0;

        // Used to disable telemetry on the fly.
        std::atomic_bool s_isTelemetryEnabled{ true };

        std::atomic_uint32_t s_executionStage{ 0 };

        std::atomic_uint32_t s_subExecutionId{ s_RootExecutionId };

        bool IsTelemetryEnabled()
        {
            return g_IsTelemetryProviderEnabled && s_isTelemetryEnabled;
        }

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
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "FailureInfo",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TraceLoggingHResult(failure.hr, "HResult"),
                TraceLoggingWideString(failure.pszMessage, "Message"),
                TraceLoggingString(failure.pszModule, "Module"),
                TraceLoggingUInt32(failure.threadId, "ThreadId"),
                TraceLoggingUInt32(static_cast<uint32_t>(failure.type), "Type"),
                TraceLoggingString(failure.pszFile, "File"),
                TraceLoggingUInt32(failure.uLineNumber, "Line"),
                TraceLoggingUInt32(s_executionStage, "ExecutionStage"),
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
        LocIndString version = Runtime::GetClientVersion();
        LocIndString packageVersion;
        if (Runtime::IsRunningInPackagedContext())
        {
            packageVersion = Runtime::GetPackageVersion();
        }

        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "ClientVersion",
                GetActivityId(),
                nullptr,
                TraceLoggingCountedString(version->c_str(), static_cast<ULONG>(version->size()), "Version"),
                TraceLoggingCountedString(packageVersion->c_str(), static_cast<ULONG>(packageVersion->size()), "PackageVersion"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance|PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(Core, Info, << "WinGet, version [" << version << "], activity [" << *GetActivityId() << ']');
        AICLI_LOG(Core, Info, << "OS: " << Runtime::GetOSVersion());
        AICLI_LOG(Core, Info, << "Command line Args: " << Utility::ConvertToUTF8(GetCommandLineW()));
        if (Runtime::IsRunningInPackagedContext())
        {
            AICLI_LOG(Core, Info, << "Package: " << packageVersion);
        }
    }
 
    void TelemetryTraceLogger::LogCommand(std::string_view commandName) noexcept
    {
        if (IsTelemetryEnabled())
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
        if (IsTelemetryEnabled())
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

    void TelemetryTraceLogger::LogCommandTermination(HRESULT hr, std::string_view file, size_t line) noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "CommandTermination",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TraceLoggingHResult(hr, "HResult"),
                AICLI_TraceLoggingStringView(file, "File"),
                TraceLoggingUInt64(static_cast<UINT64>(line), "Line"),
                TraceLoggingUInt32(s_executionStage, "ExecutionStage"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Error, << "Terminating context: 0x" << SetHRFormat << hr << " at " << file << ":" << line);
    }

    void TelemetryTraceLogger::LogException(std::string_view commandName, std::string_view type, std::string_view message) noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "Exception",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(commandName, "Command"),
                AICLI_TraceLoggingStringView(type, "Type"),
                AICLI_TraceLoggingStringView(message, "Message"),
                TraceLoggingUInt32(s_executionStage, "ExecutionStage"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Error, << "Caught " << type << ": " << message);
    }

    void TelemetryTraceLogger::LogIsManifestLocal(bool isLocalManifest) noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "GetManifest",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TraceLoggingBool(isLocalManifest, "IsManifestLocal"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }
    }

    void TelemetryTraceLogger::LogManifestFields(std::string_view id, std::string_view name, std::string_view version) noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "ManifestFields",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(name,"Name"),
                AICLI_TraceLoggingStringView(version, "Version"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance|PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Manifest fields: Name [" << name << "], Version [" << version << ']');
    }

    void TelemetryTraceLogger::LogNoAppMatch() noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "NoAppMatch",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "No app found matching input criteria");
    }

    void TelemetryTraceLogger::LogMultiAppMatch() noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "MultiAppMatch",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Multiple apps found matching input criteria");
    }

    void TelemetryTraceLogger::LogAppFound(std::string_view name, std::string_view id) noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "AppFound",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(name, "Name"),
                AICLI_TraceLoggingStringView(id, "Id"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Info, << "Found one app. App id: " << id << " App name: " << name);
    }

    void TelemetryTraceLogger::LogSelectedInstaller(int arch, std::string_view url, std::string_view installerType, std::string_view scope, std::string_view language) noexcept
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SelectedInstaller",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TraceLoggingInt32(arch, "Arch"),
                AICLI_TraceLoggingStringView(url, "Url"),
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
        std::string_view type,
        std::string_view query,
        std::string_view id,
        std::string_view name,
        std::string_view moniker,
        std::string_view tag,
        std::string_view command,
        size_t maximum,
        std::string_view request)
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SearchRequest",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(type, "Type"),
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
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "SearchResultCount",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TraceLoggingUInt64(resultCount, "ResultCount"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }
    }

    void TelemetryTraceLogger::LogInstallerHashMismatch(
        std::string_view id,
        std::string_view version,
        std::string_view channel,
        const std::vector<uint8_t>& expected,
        const std::vector<uint8_t>& actual,
        bool overrideHashMismatch)
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "HashMismatch",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(version, "Version"),
                AICLI_TraceLoggingStringView(channel, "Channel"),
                TraceLoggingBinary(expected.data(), static_cast<ULONG>(expected.size()), "Expected"),
                TraceLoggingBinary(actual.data(), static_cast<ULONG>(actual.size()), "Actual"),
                TraceLoggingValue(overrideHashMismatch, "Override"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Error,
            << "Package hash verification failed. SHA256 in manifest ["
            << Utility::SHA256::ConvertToString(expected)
            << "] does not match download ["
            << Utility::SHA256::ConvertToString(actual)
            << ']');
    }

    void TelemetryTraceLogger::LogInstallerFailure(std::string_view id, std::string_view version, std::string_view channel, std::string_view type, uint32_t errorCode)
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "InstallerFailure",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(version, "Version"),
                AICLI_TraceLoggingStringView(channel, "Channel"),
                AICLI_TraceLoggingStringView(type, "Type"),
                TraceLoggingUInt32(errorCode, "ErrorCode"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Error, << type << " installer failed: " << errorCode);
    }

    void TelemetryTraceLogger::LogDuplicateARPEntry(HRESULT hr, std::string_view scope, std::string_view architecture, std::string_view productCode, std::string_view name)
    {
        if (IsTelemetryEnabled())
        {
            TraceLoggingWriteActivity(g_hTelemetryProvider,
                "DuplicateARPEntry",
                GetActivityId(),
                nullptr,
                TraceLoggingUInt32(s_subExecutionId, "SubExecutionId"),
                TraceLoggingHResult(hr, "HResult"),
                AICLI_TraceLoggingStringView(scope, "Scope"),
                AICLI_TraceLoggingStringView(architecture, "Architecture"),
                AICLI_TraceLoggingStringView(productCode, "ProductCode"),
                AICLI_TraceLoggingStringView(name, "Name"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));
        }

        AICLI_LOG(CLI, Error, << "Ignoring duplicate ARP entry " << scope << '|' << architecture << '|' << productCode << " [" << name << "]");
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }

    DisableTelemetryScope::DisableTelemetryScope()
    {
        m_token = s_isTelemetryEnabled.exchange(false);
    }

    DisableTelemetryScope::~DisableTelemetryScope()
    {
        if (m_token)
        {
            s_isTelemetryEnabled = true;
        }
    }

    void SetExecutionStage(uint32_t stage)
    {
        s_executionStage = stage;
    }

    std::atomic_uint32_t SubExecutionTelemetryScope::m_sessionId{ s_RootExecutionId };

    SubExecutionTelemetryScope::SubExecutionTelemetryScope()
    {
        auto expected = s_RootExecutionId;
        THROW_HR_IF_MSG(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !s_subExecutionId.compare_exchange_strong(expected, ++m_sessionId),
            "Cannot create a sub execution telemetry session when a previous session exists.");
    }

    SubExecutionTelemetryScope::~SubExecutionTelemetryScope()
    {
        s_subExecutionId = s_RootExecutionId;
    }
}