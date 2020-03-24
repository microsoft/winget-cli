// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerSQLiteIndexUtil.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Microsoft;

extern "C"
{
    APPINSTALLER_SQLITE_INDEX_API AppInstallerLoggingInit(APPINSTALLER_SQLITE_INDEX_STRING logPath) try
    {
        THROW_HR_IF(E_INVALIDARG, !logPath);

        static std::once_flag s_initLogging;
        std::call_once(s_initLogging, []() {
            // Enable all logs for now.
            AppInstaller::Logging::Log().EnableChannel(AppInstaller::Logging::Channel::All);
            AppInstaller::Logging::Log().SetLevel(AppInstaller::Logging::Level::Verbose);
            AppInstaller::Logging::EnableWilFailureTelemetry();
            });

        std::filesystem::path pathAsPath = logPath;
        std::string loggerName = AppInstaller::Logging::FileLogger::GetNameForPath(pathAsPath);

        if (!AppInstaller::Logging::Log().ContainsLogger(loggerName))
        {
            AppInstaller::Logging::AddFileLogger(pathAsPath);
        }

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerLoggingTerm(APPINSTALLER_SQLITE_INDEX_STRING logPath) try
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

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexCreate(APPINSTALLER_SQLITE_INDEX_STRING filePath, UINT32 majorVersion, UINT32 minorVersion, APPINSTALLER_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);
        Schema::Version internalVersion{ majorVersion, minorVersion };

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::CreateNew(filePathUtf8, internalVersion));

        *index = static_cast<APPINSTALLER_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexOpen(APPINSTALLER_SQLITE_INDEX_STRING filePath, APPINSTALLER_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::Open(filePathUtf8, SQLiteIndex::OpenDisposition::ReadWrite));

        *index = static_cast<APPINSTALLER_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexClose(APPINSTALLER_SQLITE_INDEX_HANDLE index) try
    {
        std::unique_ptr<SQLiteIndex> toClose(reinterpret_cast<SQLiteIndex*>(index));

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexAddManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, APPINSTALLER_SQLITE_INDEX_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->AddManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexUpdateManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index,
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath,
        APPINSTALLER_SQLITE_INDEX_STRING relativePath,
        bool* indexModified) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        bool result = reinterpret_cast<SQLiteIndex*>(index)->UpdateManifest(manifestPath, relativePath);
        if (indexModified)
        {
            *indexModified = result;
        }

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexRemoveManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, APPINSTALLER_SQLITE_INDEX_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->RemoveManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexPrepareForPackaging(
        APPINSTALLER_SQLITE_INDEX_HANDLE index) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);

        reinterpret_cast<SQLiteIndex*>(index)->PrepareForPackaging();

        return S_OK;
    }
    CATCH_RETURN()

    APPINSTALLER_SQLITE_INDEX_API AppInstallerValidateManifest(
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath,
        bool* succeeded,
        APPINSTALLER_SQLITE_INDEX_STRING_OUT* failureMessage) try
    {
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !succeeded);

        try
        {
            (void)Manifest::CreateFromPath(manifestPath, true);
            *succeeded = true;
        }
        catch (const ManifestException& e)
        {
            *succeeded = false;
            if (failureMessage)
            {
                *failureMessage = ::SysAllocString(ConvertToUTF16(e.GetManifestErrorMessage()).c_str());
            }
        }

        return S_OK;
    }
    CATCH_RETURN()
}
