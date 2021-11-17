// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WinGetUtil.h"

#include <AppInstallerDownloader.h>
#include <AppInstallerFileLogger.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>
#include <AppInstallerTelemetry.h>
#include <Microsoft/SQLiteIndex.h>
#include <winget/ManifestYamlParser.h>
#include <PackageDependenciesValidation.h>
#include <winget/ThreadGlobals.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;

extern "C"
{
    WINGET_UTIL_API WinGetLoggingInit(WINGET_STRING logPath) try
    {
        THROW_HR_IF(E_INVALIDARG, !logPath);

        thread_local AppInstaller::ThreadLocalStorage::ThreadGlobals threadGlobals;
        thread_local std::once_flag initLogging;

        std::call_once(initLogging, []() {
            std::unique_ptr<AppInstaller::ThreadLocalStorage::PreviousThreadGlobals> previous = threadGlobals.SetForCurrentThread();
            // Intentionally release to leave the local ThreadGlobals.
            previous.release();
            // Enable all logs for now.
            AppInstaller::Logging::Log().EnableChannel(AppInstaller::Logging::Channel::All);
            AppInstaller::Logging::Log().SetLevel(AppInstaller::Logging::Level::Verbose);
            AppInstaller::Logging::EnableWilFailureTelemetry();
            });

        std::filesystem::path pathAsPath = logPath;
        std::string loggerName = AppInstaller::Logging::FileLogger::GetNameForPath(pathAsPath);

        if (!AppInstaller::Logging::Log().ContainsLogger(loggerName))
        {
            // Let FileLogger use default file prefix
            AppInstaller::Logging::AddFileLogger(pathAsPath);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetLoggingTerm(WINGET_STRING logPath) try
    {
        if (logPath)
        {
            std::string loggerName = AppInstaller::Logging::FileLogger::GetNameForPath(logPath);
            (void)AppInstaller::Logging::Log().RemoveLogger(loggerName);
        }
        else
        {
            AppInstaller::Logging::Log().RemoveAllLoggers();
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexCreate(WINGET_STRING filePath, UINT32 majorVersion, UINT32 minorVersion, WINGET_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);
        Schema::Version internalVersion{ majorVersion, minorVersion };

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::CreateNew(filePathUtf8, internalVersion));

        *index = static_cast<WINGET_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexOpen(WINGET_STRING filePath, WINGET_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::Open(filePathUtf8, SQLiteIndex::OpenDisposition::ReadWrite));

        *index = static_cast<WINGET_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexClose(WINGET_SQLITE_INDEX_HANDLE index) try
    {
        std::unique_ptr<SQLiteIndex> toClose(reinterpret_cast<SQLiteIndex*>(index));

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexAddManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, WINGET_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->AddManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexUpdateManifest(
        WINGET_SQLITE_INDEX_HANDLE index,
        WINGET_STRING manifestPath,
        WINGET_STRING relativePath,
        BOOL* indexModified) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        bool result = reinterpret_cast<SQLiteIndex*>(index)->UpdateManifest(manifestPath, relativePath);
        if (indexModified)
        {
            *indexModified = (result ? TRUE : FALSE);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexRemoveManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, WINGET_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->RemoveManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexPrepareForPackaging(
        WINGET_SQLITE_INDEX_HANDLE index) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);

        reinterpret_cast<SQLiteIndex*>(index)->PrepareForPackaging();

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetSQLiteIndexCheckConsistency(
        WINGET_SQLITE_INDEX_HANDLE index,
        BOOL* succeeded) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        bool result = reinterpret_cast<SQLiteIndex*>(index)->CheckConsistency(true);

        *succeeded = (result ? TRUE : FALSE);

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifest(
        WINGET_STRING manifestPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message) try
    {
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            ManifestValidateOption validateOption;
            validateOption.FullValidation = true;
            validateOption.ThrowOnWarning = true;

            (void)YamlParser::CreateFromPath(manifestPath, validateOption);

            *succeeded = TRUE;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifestV2(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message,
        WINGET_STRING mergedManifestPath,
        WinGetValidateManifestOption option) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            ManifestValidateOption validateOption;
            validateOption.FullValidation = true;
            validateOption.ThrowOnWarning = true;
            validateOption.SchemaValidationOnly = WI_IsFlagSet(option, WinGetValidateManifestOption::SchemaValidationOnly);
            validateOption.ErrorOnVerifiedPublisherFields = WI_IsFlagSet(option, WinGetValidateManifestOption::ErrorOnVerifiedPublisherFields);

            (void)YamlParser::CreateFromPath(inputPath, validateOption, mergedManifestPath ? mergedManifestPath : L"");

            *succeeded = TRUE;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetValidateManifestDependencies(
        WINGET_STRING inputPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message,
        WINGET_SQLITE_INDEX_HANDLE index,
        WinGetValidateManifestDependenciesOption validationOption) try
    {
        THROW_HR_IF(E_INVALIDARG, !inputPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            Manifest manifest = YamlParser::CreateFromPath(inputPath);
            SQLiteIndex* sqliteIndex(reinterpret_cast<SQLiteIndex*>(index));
            
            switch (validationOption)
            {
                case WinGetValidateManifestDependenciesOption::DefaultValidation:
                    PackageDependenciesValidation::ValidateManifestDependencies(sqliteIndex, manifest);
                    break;
                case WinGetValidateManifestDependenciesOption::ForDelete:
                    PackageDependenciesValidation::VerifyDependenciesStructureForManifestDelete(sqliteIndex, manifest);
                    break;
                default:
                    THROW_HR(E_INVALIDARG);
            }

            *succeeded = TRUE;
        }
        catch (const ManifestException& e)
        {
            *succeeded = e.IsWarningOnly();
            if (message)
            {
                *message = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetDownload(
        WINGET_STRING url,
        WINGET_STRING filePath,
        BYTE* sha256Hash,
        UINT32 sha256HashLength) try
    {
        THROW_HR_IF(E_INVALIDARG, !url);
        THROW_HR_IF(E_INVALIDARG, !filePath);

        bool computeHash = sha256Hash != nullptr && sha256HashLength != 0;
        THROW_HR_IF(E_INVALIDARG, !computeHash && (sha256Hash != nullptr || sha256HashLength != 0));
        THROW_HR_IF(E_INVALIDARG, computeHash && sha256HashLength != 32);

        AppInstaller::ProgressCallback callback;
        auto hashValue = Download(ConvertToUTF8(url), filePath, DownloadType::WinGetUtil, callback, computeHash);

        // At this point, if computeHash is set we have verified that the buffer is valid and 32 bytes.
        if (computeHash)
        {
            const auto& hash = hashValue.value();

            // The SHA 256 hash length should always be 32 bytes.
            THROW_HR_IF(E_UNEXPECTED, hash.size() != sha256HashLength);
            std::copy(hash.begin(), hash.end(), sha256Hash);
        }

        return S_OK;
    }
    CATCH_RETURN()

    WINGET_UTIL_API WinGetCompareVersions(
        WINGET_STRING versionA,
        WINGET_STRING versionB,
        INT* comparisonResult) try
    {
        THROW_HR_IF(E_INVALIDARG, !versionA);
        THROW_HR_IF(E_INVALIDARG, !versionB);

        Version vA{ ConvertToUTF8(versionA) };
        Version vB{ ConvertToUTF8(versionB) };

        *comparisonResult = vA < vB ? -1 : (vA == vB ? 0 : 1);

        return S_OK;
    }
    CATCH_RETURN()
}
