// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerSQLiteIndexUtil.h"


using namespace AppInstaller::Utility;
using namespace AppInstaller::Repository::Microsoft;

extern "C"
{
    HRESULT AppInstallerSQLiteIndexCreate(APPINSTALLER_SQLITE_INDEX_STRING filePath, AppInstallerSQLiteIndexVersion version, APPINSTALLER_SQLITE_INDEX_HANDLE* index) try
    {
        THROW_HR_IF(E_INVALIDARG, !filePath);
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !!*index);

        std::string filePathUtf8 = ConvertToUTF8(filePath);
        Schema::Version internalVersion{ version.MajorVersion, version.MinorVersion };

        std::unique_ptr<SQLiteIndex> result = std::make_unique<SQLiteIndex>(SQLiteIndex::CreateNew(filePathUtf8, internalVersion));

        *index = static_cast<APPINSTALLER_SQLITE_INDEX_HANDLE>(result.release());

        return S_OK;
    }
    CATCH_RETURN()

    HRESULT AppInstallerSQLiteIndexOpen(APPINSTALLER_SQLITE_INDEX_STRING filePath, APPINSTALLER_SQLITE_INDEX_HANDLE* index) try
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

    HRESULT AppInstallerSQLiteIndexClose(APPINSTALLER_SQLITE_INDEX_HANDLE index) try
    {
        std::unique_ptr<SQLiteIndex> toClose(reinterpret_cast<SQLiteIndex*>(index));

        return S_OK;
    }
    CATCH_RETURN()

    HRESULT AppInstallerSQLiteIndexAddManifest(APPINSTALLER_SQLITE_INDEX_HANDLE index, APPINSTALLER_SQLITE_INDEX_STRING manifestPath, APPINSTALLER_SQLITE_INDEX_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->AddManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()

    HRESULT AppInstallerSQLiteIndexUpdateManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index,
        APPINSTALLER_SQLITE_INDEX_STRING oldManifestPath, APPINSTALLER_SQLITE_INDEX_STRING oldRelativePath,
        APPINSTALLER_SQLITE_INDEX_STRING newManifestPath, APPINSTALLER_SQLITE_INDEX_STRING newRelativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !oldManifestPath);
        THROW_HR_IF(E_INVALIDARG, !oldRelativePath);
        THROW_HR_IF(E_INVALIDARG, !newManifestPath);
        THROW_HR_IF(E_INVALIDARG, !newRelativePath);

        reinterpret_cast<SQLiteIndex*>(index)->UpdateManifest(oldManifestPath, oldRelativePath, newManifestPath, newRelativePath);

        return S_OK;
    }
    CATCH_RETURN()

    HRESULT AppInstallerSQLiteIndexRemoveManifest(APPINSTALLER_SQLITE_INDEX_HANDLE index, APPINSTALLER_SQLITE_INDEX_STRING manifestPath, APPINSTALLER_SQLITE_INDEX_STRING relativePath) try
    {
        THROW_HR_IF(E_INVALIDARG, !index);
        THROW_HR_IF(E_INVALIDARG, !manifestPath);
        THROW_HR_IF(E_INVALIDARG, !relativePath);

        reinterpret_cast<SQLiteIndex*>(index)->RemoveManifest(manifestPath, relativePath);

        return S_OK;
    }
    CATCH_RETURN()
}
