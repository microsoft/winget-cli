// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerTelemetry.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/AppInstallerStrings.h"
#include "winget/UserSettings.h"
#include "Public/winget/ThreadGlobals.h"

#define AICLI_TraceLoggingStringView(_sv_,_name_) TraceLoggingCountedUtf8String(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)
#define AICLI_TraceLoggingWStringView(_sv_,_name_) TraceLoggingCountedWideString(_sv_.data(), static_cast<ULONG>(_sv_.size()), _name_)

#define AICLI_TraceLoggingWriteActivity(_eventName_,...) TraceLoggingWriteActivity(\
g_hTraceProvider,\
_eventName_,\
s_useGlobalTelemetryActivityId ? s_globalTelemetryLoggerActivityId : GetActivityId(),\
nullptr,\
TraceLoggingCountedUtf8String(m_caller.c_str(),  static_cast<ULONG>(m_caller.size()), "Caller"),\
TraceLoggingPackedFieldEx(m_telemetryCorrelationJsonW.c_str(), static_cast<ULONG>((m_telemetryCorrelationJsonW.size() + 1) * sizeof(wchar_t)), TlgInUNICODESTRING, TlgOutJSON, "CvJson"),\
__VA_ARGS__)

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
        // TODO: This and all usages should be removed after transition to summary event in back end.
        static const uint32_t s_RootExecutionId = 0;
        static std::atomic_uint32_t s_subExecutionId{ s_RootExecutionId };

        constexpr std::wstring_view s_UserProfileReplacement = L"%USERPROFILE%"sv;

        // TODO: Temporary code to keep existing telemetry behavior
        static bool s_useGlobalTelemetryActivityId = false;
        static const GUID* s_globalTelemetryLoggerActivityId = nullptr;

        void __stdcall wilResultLoggingCallback(const wil::FailureInfo& info) noexcept
        {
            Telemetry().LogFailure(info);
        }

        UINT32 ConvertWilFailureTypeToFailureType(wil::FailureType failureType)
        {
            switch (failureType)
            {
            case wil::FailureType::Exception:
                return FailureTypeResultException;
            case wil::FailureType::Return:
                return FailureTypeResultReturn;
            case wil::FailureType::Log:
                return FailureTypeResultLog;
            case wil::FailureType::FailFast:
                return FailureTypeResultFailFast;
            default:
                return FailureTypeUnknown;
            }
        }

        std::string_view LogExceptionTypeToString(UINT32 exceptionType)
        {
            switch (exceptionType)
            {
            case FailureTypeResultException:
                return "wil::ResultException"sv;
            case FailureTypeWinrtHResultError:
                return "winrt::hresult_error"sv;
            case FailureTypeResourceOpen:
                return "ResourceOpenException"sv;
            case FailureTypeStdException:
                return "std::exception"sv;
            case FailureTypeUnknown:
            default:
                return "unknown"sv;
            }
        }
    }

    TelemetryTraceLogger::TelemetryTraceLogger()
    {
        std::ignore = CoCreateGuid(&m_activityId);
        m_subExecutionId = s_RootExecutionId;
    }

    const GUID* TelemetryTraceLogger::GetActivityId() const
    {
        return &m_activityId;
    }

    const GUID* TelemetryTraceLogger::GetParentActivityId() const
    {
        return &m_parentActivityId;
    }

    bool TelemetryTraceLogger::DisableRuntime()
    {
        return m_isRuntimeEnabled.exchange(false);
    }

    void TelemetryTraceLogger::EnableRuntime()
    {
        m_isRuntimeEnabled = true;
    }

    void TelemetryTraceLogger::Initialize()
    {
        if (!m_isInitialized)
        {
            InitializeInternal(Settings::User());
        }
    }

    bool TelemetryTraceLogger::TryInitialize()
    {
        if (!m_isInitialized)
        {
            auto userSettings = Settings::TryGetUser();
            if (userSettings)
            {
                InitializeInternal(*userSettings);
            }
        }

        return m_isInitialized;
    }

    void TelemetryTraceLogger::SetTelemetryCorrelationJson(const std::wstring_view jsonStr_view) noexcept
    {
        // Check if passed in string is a valid Json formatted before returning the value
        // If invalid, return empty Json
        Json::CharReaderBuilder jsonBuilder;
        std::unique_ptr<Json::CharReader> jsonReader(jsonBuilder.newCharReader());
        std::unique_ptr<Json::Value> pJsonValue = std::make_unique<Json::Value>();
        std::string errors;
        std::wstring jsonStrW{ jsonStr_view };
        std::string jsonStr = ConvertToUTF8(jsonStrW.c_str());

        bool result = jsonReader->parse(jsonStr.c_str(),
            jsonStr.c_str() + jsonStr.size(),
            pJsonValue.get(),
            &errors);

        if (result)
        {
            m_telemetryCorrelationJsonW = jsonStrW;
            AICLI_LOG(Core, Info, << "Passed in Correlation Vector Json is valid: " << jsonStr);
        }
        else
        {
            AICLI_LOG(Core, Error, << "Passed in Correlation Vector Json is invalid: " << jsonStr << "; Error: " << errors);
        }
    }

    void TelemetryTraceLogger::SetCaller(const std::string& caller)
    {
        m_caller = caller;
    }

    void TelemetryTraceLogger::SetExecutionStage(uint32_t stage) noexcept
    {
        m_executionStage = stage;
    }

    std::unique_ptr<TelemetryTraceLogger> TelemetryTraceLogger::CreateSubTraceLogger() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !this->m_isInitialized);

        auto subTraceLogger = std::make_unique<TelemetryTraceLogger>();

        subTraceLogger->m_parentActivityId = this->m_activityId;
        subTraceLogger->m_subExecutionId = s_subExecutionId++;

        // Copy remaining fields from parent.
        subTraceLogger->m_isSettingEnabled = this->m_isSettingEnabled;
        subTraceLogger->m_isRuntimeEnabled = this->m_isRuntimeEnabled.load();
        subTraceLogger->m_isInitialized = this->m_isInitialized.load();
        subTraceLogger->m_executionStage = this->m_executionStage.load();
        subTraceLogger->m_telemetryCorrelationJsonW = this->m_telemetryCorrelationJsonW;
        subTraceLogger->m_caller = this->m_caller;
        subTraceLogger->m_userProfile = this->m_userProfile;
        subTraceLogger->m_summary = this->m_summary;

        return subTraceLogger;
    }

    void TelemetryTraceLogger::LogFailure(const wil::FailureInfo& failure) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            auto anonMessage = AnonymizeString(failure.pszMessage);

            AICLI_TraceLoggingWriteActivity(
                "FailureInfo",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TraceLoggingHResult(failure.hr, "HResult"),
                AICLI_TraceLoggingWStringView(anonMessage, "Message"),
                TraceLoggingString(failure.pszModule, "Module"),
                TraceLoggingUInt32(failure.threadId, "ThreadId"),
                TraceLoggingUInt32(static_cast<uint32_t>(failure.type), "Type"),
                TraceLoggingString(failure.pszFile, "File"),
                TraceLoggingUInt32(failure.uLineNumber, "Line"),
                TraceLoggingUInt32(m_executionStage, "ExecutionStage"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.FailureHResult = failure.hr;
            m_summary.FailureMessage = anonMessage;
            m_summary.FailureModule = failure.pszModule;
            m_summary.FailureThreadId = failure.threadId;
            m_summary.FailureType = ConvertWilFailureTypeToFailureType(failure.type);
            m_summary.FailureFile = failure.pszFile;
            m_summary.FailureLine = failure.uLineNumber;
        }

        // Also send failure to the log
        AICLI_LOG(Fail, Error, << [&]() {
            wchar_t message[2048];
            GetFailureLogString(message, ARRAYSIZE(message), failure);
            return Utility::ConvertToUTF8(message);
            }());
    }

    void TelemetryTraceLogger::LogStartup(bool isCOMCall) const noexcept
    {
        LocIndString version = Runtime::GetClientVersion();
        LocIndString packageVersion;
        if (Runtime::IsRunningInPackagedContext())
        {
            packageVersion = Runtime::GetPackageVersion();
        }

        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "ClientVersion",
                TraceLoggingBool(isCOMCall, "IsCOMCall"),
                TraceLoggingCountedString(version->c_str(), static_cast<ULONG>(version->size()), "Version"),
                TraceLoggingCountedString(packageVersion->c_str(), static_cast<ULONG>(packageVersion->size()), "PackageVersion"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.IsCOMCall = isCOMCall;
        }

        AICLI_LOG(Core, Info, << "WinGet, version [" << version << "], activity [" << *GetActivityId() << ']');
        AICLI_LOG(Core, Info, << "OS: " << Runtime::GetOSVersion());
        AICLI_LOG(Core, Info, << "Command line Args: " << Utility::ConvertToUTF8(GetCommandLineW()));
        if (Runtime::IsRunningInPackagedContext())
        {
            AICLI_LOG(Core, Info, << "Package: " << packageVersion);
        }
        AICLI_LOG(Core, Info, << "IsCOMCall:" << isCOMCall << "; Caller: " << m_caller);
    }

    void TelemetryTraceLogger::LogCommand(std::string_view commandName) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "CommandFound",
                AICLI_TraceLoggingStringView(commandName, "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.Command = commandName;
        }

        AICLI_LOG(CLI, Info, << "Leaf command to execute: " << commandName);
    }

    void TelemetryTraceLogger::LogCommandSuccess(std::string_view commandName) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "CommandSuccess",
                AICLI_TraceLoggingStringView(commandName, "Command"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.CommandSuccess = true;
        }

        AICLI_LOG(CLI, Info, << "Leaf command succeeded: " << commandName);
    }

    void TelemetryTraceLogger::LogCommandTermination(HRESULT hr, std::string_view file, size_t line) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "CommandTermination",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TraceLoggingHResult(hr, "HResult"),
                AICLI_TraceLoggingStringView(file, "File"),
                TraceLoggingUInt64(static_cast<UINT64>(line), "Line"),
                TraceLoggingUInt32(m_executionStage, "ExecutionStage"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.CommandTerminationHResult = hr;
            m_summary.CommandTerminationFile = file;
            m_summary.CommandTerminationLine = static_cast<UINT64>(line);
        }

        AICLI_LOG(CLI, Error, << "Terminating context: 0x" << SetHRFormat << hr << " at " << file << ":" << line);
    }

    void TelemetryTraceLogger::LogException(UINT32 type, std::string_view message) const noexcept
    {
        auto exceptionTypeString = LogExceptionTypeToString(type);

        if (IsTelemetryEnabled())
        {
            auto anonMessage = AnonymizeString(Utility::ConvertToUTF16(message));

            AICLI_TraceLoggingWriteActivity(
                "Exception",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(exceptionTypeString, "Type"),
                AICLI_TraceLoggingWStringView(anonMessage, "Message"),
                TraceLoggingUInt32(m_executionStage, "ExecutionStage"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.FailureType = type;
            m_summary.FailureMessage = anonMessage;
        }

        AICLI_LOG(CLI, Error, << "Caught " << exceptionTypeString << ": " << message);
    }

    void TelemetryTraceLogger::LogIsManifestLocal(bool isLocalManifest) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "GetManifest",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TraceLoggingBool(isLocalManifest, "IsManifestLocal"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.IsManifestLocal = isLocalManifest;
        }
    }

    void TelemetryTraceLogger::LogManifestFields(std::string_view id, std::string_view name, std::string_view version) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "ManifestFields",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(name, "Name"),
                AICLI_TraceLoggingStringView(version, "Version"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.PackageIdentifier = id;
            m_summary.PackageName = name;
            m_summary.PackageVersion = version;
        }

        AICLI_LOG(CLI, Info, << "Manifest fields: Name [" << name << "], Version [" << version << ']');
    }

    void TelemetryTraceLogger::LogNoAppMatch() const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "NoAppMatch",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.NoAppMatch = true;
        }

        AICLI_LOG(CLI, Info, << "No app found matching input criteria");
    }

    void TelemetryTraceLogger::LogMultiAppMatch() const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "MultiAppMatch",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.MultipleAppMatch = true;
        }

        AICLI_LOG(CLI, Info, << "Multiple apps found matching input criteria");
    }

    void TelemetryTraceLogger::LogAppFound(std::string_view name, std::string_view id) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "AppFound",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(name, "Name"),
                AICLI_TraceLoggingStringView(id, "Id"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.PackageIdentifier = id;
            m_summary.PackageName = name;
        }

        AICLI_LOG(CLI, Info, << "Found one app. App id: " << id << " App name: " << name);
    }

    void TelemetryTraceLogger::LogSelectedInstaller(int arch, std::string_view url, std::string_view installerType, std::string_view scope, std::string_view language) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "SelectedInstaller",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TraceLoggingInt32(arch, "Arch"),
                AICLI_TraceLoggingStringView(url, "Url"),
                AICLI_TraceLoggingStringView(installerType, "InstallerType"),
                AICLI_TraceLoggingStringView(scope, "Scope"),
                AICLI_TraceLoggingStringView(language, "Language"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.InstallerArchitecture = arch;
            m_summary.InstallerUrl = url;
            m_summary.InstallerType = installerType;
            m_summary.InstallerScope = scope;
            m_summary.InstallerLocale = language;
        }

        AICLI_LOG(CLI, Info, << "Completed installer selection.");
        AICLI_LOG(CLI, Verbose, << "Selected installer Architecture: " << arch);
        AICLI_LOG(CLI, Verbose, << "Selected installer URL: " << url);
        AICLI_LOG(CLI, Verbose, << "Selected installer InstallerType: " << installerType);
        AICLI_LOG(CLI, Verbose, << "Selected installer Scope: " << scope);
        AICLI_LOG(CLI, Verbose, << "Selected installer Language: " << language);
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
        std::string_view request) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "SearchRequest",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(type, "Type"),
                AICLI_TraceLoggingStringView(query, "Query"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(name, "Name"),
                AICLI_TraceLoggingStringView(moniker, "Moniker"),
                AICLI_TraceLoggingStringView(tag, "Tag"),
                AICLI_TraceLoggingStringView(command, "Command"),
                TraceLoggingUInt64(static_cast<UINT64>(maximum), "Maximum"),
                AICLI_TraceLoggingStringView(request, "Request"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.SearchType = type;
            m_summary.SearchQuery = query;
            m_summary.SearchId = id;
            m_summary.SearchName = name;
            m_summary.SearchMoniker = moniker;
            m_summary.SearchTag = tag;
            m_summary.SearchCommand = command;
            m_summary.SearchMaximum = static_cast<UINT64>(maximum);
            m_summary.SearchRequest = request;
        }
    }

    void TelemetryTraceLogger::LogSearchResultCount(uint64_t resultCount) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "SearchResultCount",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                TraceLoggingUInt64(resultCount, "ResultCount"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.SearchResultCount = resultCount;
        }
    }

    void TelemetryTraceLogger::LogInstallerHashMismatch(
        std::string_view id,
        std::string_view version,
        std::string_view channel,
        const std::vector<uint8_t>& expected,
        const std::vector<uint8_t>& actual,
        bool overrideHashMismatch) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "HashMismatch",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(version, "Version"),
                AICLI_TraceLoggingStringView(channel, "Channel"),
                TraceLoggingBinary(expected.data(), static_cast<ULONG>(expected.size()), "Expected"),
                TraceLoggingBinary(actual.data(), static_cast<ULONG>(actual.size()), "Actual"),
                TraceLoggingBool(overrideHashMismatch, "Override"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.PackageIdentifier = id;
            m_summary.PackageVersion = version;
            m_summary.Channel = channel;
            m_summary.HashMismatchExpected = expected;
            m_summary.HashMismatchActual = actual;
            m_summary.HashMismatchOverride = overrideHashMismatch;
        }

        AICLI_LOG(CLI, Error,
            << "Package hash verification failed. SHA256 in manifest ["
            << Utility::SHA256::ConvertToString(expected)
            << "] does not match download ["
            << Utility::SHA256::ConvertToString(actual)
            << ']');
    }

    void TelemetryTraceLogger::LogInstallerFailure(std::string_view id, std::string_view version, std::string_view channel, std::string_view type, uint32_t errorCode) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "InstallerFailure",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(version, "Version"),
                AICLI_TraceLoggingStringView(channel, "Channel"),
                AICLI_TraceLoggingStringView(type, "Type"),
                TraceLoggingUInt32(errorCode, "ErrorCode"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.PackageIdentifier = id;
            m_summary.PackageVersion = version;
            m_summary.Channel = channel;
            m_summary.InstallerExecutionType = type;
            m_summary.InstallerErrorCode = errorCode;
        }

        AICLI_LOG(CLI, Error, << type << " installer failed: " << errorCode);
    }

    void TelemetryTraceLogger::LogUninstallerFailure(std::string_view id, std::string_view version, std::string_view type, uint32_t errorCode) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "UninstallerFailure",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(id, "Id"),
                AICLI_TraceLoggingStringView(version, "Version"),
                AICLI_TraceLoggingStringView(type, "Type"),
                TraceLoggingUInt32(errorCode, "ErrorCode"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.PackageIdentifier = id;
            m_summary.PackageVersion = version;
            m_summary.UninstallerExecutionType = type;
            m_summary.UninstallerErrorCode = errorCode;
        }

        AICLI_LOG(CLI, Error, << type << " uninstaller failed: " << errorCode);
    }

    void TelemetryTraceLogger::LogSuccessfulInstallARPChange(
        std::string_view sourceIdentifier,
        std::string_view packageIdentifier,
        std::string_view packageVersion,
        std::string_view packageChannel,
        size_t changesToARP,
        size_t matchesInARP,
        size_t countOfIntersectionOfChangesAndMatches,
        std::string_view arpName,
        std::string_view arpVersion,
        std::string_view arpPublisher,
        std::string_view arpLanguage) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            size_t languageNumber = 0xFFFF;

            try
            {
                std::istringstream languageConversion{ std::string{ arpLanguage } };
                languageConversion >> languageNumber;
            }
            catch (...) {}

            AICLI_TraceLoggingWriteActivity(
                "InstallARPChange",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(sourceIdentifier, "SourceIdentifier"),
                AICLI_TraceLoggingStringView(packageIdentifier, "PackageIdentifier"),
                AICLI_TraceLoggingStringView(packageVersion, "PackageVersion"),
                AICLI_TraceLoggingStringView(packageChannel, "PackageChannel"),
                TraceLoggingUInt64(static_cast<UINT64>(changesToARP), "ChangesToARP"),
                TraceLoggingUInt64(static_cast<UINT64>(matchesInARP), "MatchesInARP"),
                TraceLoggingUInt64(static_cast<UINT64>(countOfIntersectionOfChangesAndMatches), "ChangesThatMatch"),
                AICLI_TraceLoggingStringView(arpName, "ARPName"),
                AICLI_TraceLoggingStringView(arpVersion, "ARPVersion"),
                AICLI_TraceLoggingStringView(arpPublisher, "ARPPublisher"),
                TraceLoggingUInt64(static_cast<UINT64>(languageNumber), "ARPLanguage"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance | PDT_ProductAndServiceUsage | PDT_SoftwareSetupAndInventory),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_CRITICAL_DATA));

            m_summary.SourceIdentifier = sourceIdentifier;
            m_summary.PackageIdentifier = packageIdentifier;
            m_summary.PackageVersion = packageVersion;
            m_summary.Channel = packageChannel;
            m_summary.ChangesToARP = static_cast<UINT64>(changesToARP);
            m_summary.MatchesInARP = static_cast<UINT64>(matchesInARP);
            m_summary.ChangesThatMatch = static_cast<UINT64>(countOfIntersectionOfChangesAndMatches);
            m_summary.ARPName = arpName;
            m_summary.ARPVersion = arpVersion;
            m_summary.ARPPublisher = arpPublisher;
            m_summary.ARPLanguage = static_cast<UINT64>(languageNumber);
        }

        AICLI_LOG(CLI, Info, << "During package install, " << changesToARP << " changes to ARP were observed, "
            << matchesInARP << " matches were found for the package, and " << countOfIntersectionOfChangesAndMatches << " packages were in both");

        if (arpName.empty())
        {
            AICLI_LOG(CLI, Info, << "No single entry was determined to be associated with the package");
        }
        else
        {
            AICLI_LOG(CLI, Info, << "The entry determined to be associated with the package is '" << arpName << "', with publisher '" << arpPublisher << "'");
        }
    }

    void TelemetryTraceLogger::LogNonFatalDOError(std::string_view url, HRESULT hr) const noexcept
    {
        if (IsTelemetryEnabled())
        {
            AICLI_TraceLoggingWriteActivity(
                "NonFatalDOError",
                TraceLoggingUInt32(m_subExecutionId, "SubExecutionId"),
                AICLI_TraceLoggingStringView(url, "Url"),
                TraceLoggingHResult(hr, "HResult"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));

            m_summary.DOUrl = url;
            m_summary.DOHResult = hr;
        }
    }

    bool TelemetryTraceLogger::IsTelemetryEnabled() const noexcept
    {
        return g_IsTelemetryProviderEnabled && m_isInitialized && m_isSettingEnabled && m_isRuntimeEnabled;
    }

    void TelemetryTraceLogger::InitializeInternal(const AppInstaller::Settings::UserSettings& userSettings)
    {
        m_isSettingEnabled = !userSettings.Get<Settings::Setting::TelemetryDisable>();
        m_userProfile = Runtime::GetPathTo(Runtime::PathName::UserProfile).wstring();
        m_isInitialized = true;
    }

    std::wstring TelemetryTraceLogger::AnonymizeString(const wchar_t* input) const noexcept
    {
        return input ? AnonymizeString(std::wstring_view{ input }) : std::wstring{};
    }

    std::wstring TelemetryTraceLogger::AnonymizeString(std::wstring_view input) const noexcept try
    {
        return Utility::ReplaceWhileCopying(input, m_userProfile, s_UserProfileReplacement);
    }
    catch (...) { return std::wstring{ input }; }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static std::shared_ptr<TelemetryTraceLogger> s_TelemetryTraceLogger_TestOverride;
#endif

    TelemetryTraceLogger& Telemetry()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_TelemetryTraceLogger_TestOverride)
        {
            return *s_TelemetryTraceLogger_TestOverride.get();
        }
#endif
        ThreadLocalStorage::ThreadGlobals* pThreadGlobals = ThreadLocalStorage::ThreadGlobals::GetForCurrentThread();
        if (pThreadGlobals && pThreadGlobals->ContainsTelemetryLogger())
        {
            return pThreadGlobals->GetTelemetryLogger();
        }
        else
        {
            static TelemetryTraceLogger processGlobalTelemetry;
            processGlobalTelemetry.TryInitialize();

            if (s_useGlobalTelemetryActivityId && s_globalTelemetryLoggerActivityId == nullptr)
            {
                s_globalTelemetryLoggerActivityId = processGlobalTelemetry.GetActivityId();
            }

            return processGlobalTelemetry;
        }
    }

    void EnableWilFailureTelemetry()
    {
        wil::SetResultLoggingCallback(wilResultLoggingCallback);
    }

    void UseGlobalTelemetryLoggerActivityIdOnly()
    {
        s_useGlobalTelemetryActivityId = true;
    }

    DisableTelemetryScope::DisableTelemetryScope()
    {
        m_token = Telemetry().DisableRuntime();
    }

    DisableTelemetryScope::~DisableTelemetryScope()
    {
        if (m_token)
        {
            Telemetry().EnableRuntime();
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    // Replace this test hook with context telemetry when it gets moved over
    void TestHook_SetTelemetryOverride(std::shared_ptr<TelemetryTraceLogger> ttl)
    {
        s_TelemetryTraceLogger_TestOverride = std::move(ttl);
    }
#endif
}
