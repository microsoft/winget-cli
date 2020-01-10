// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

extern "C"
{
    // The index schema version.
    struct AppInstallerSQLiteIndexVersion
    {
        UINT32 MajorVersion;
        UINT32 MinorVersion;
    };

#define APPINSTALLER_SQLITE_INDEX_VERSION_LATEST ((UINT32)-1)

    // A handle to the index.
    typedef void* APPINSTALLER_SQLITE_INDEX_HANDLE;

    // A string taken in by the utility; in UTF16.
    typedef wchar_t const* const APPINSTALLER_SQLITE_INDEX_STRING;

    // Creates a new index file at filePath with the given version.
    HRESULT AppInstallerSQLiteIndexCreate(
        APPINSTALLER_SQLITE_INDEX_STRING filePath, 
        AppInstallerSQLiteIndexVersion version, 
        APPINSTALLER_SQLITE_INDEX_HANDLE* index);

    // Opens an existing index at filePath.
    HRESULT AppInstallerSQLiteIndexOpen(
        APPINSTALLER_SQLITE_INDEX_STRING filePath, 
        APPINSTALLER_SQLITE_INDEX_HANDLE* index);

    // Closes the index.
    HRESULT AppInstallerSQLiteIndexClose(
        APPINSTALLER_SQLITE_INDEX_HANDLE index);

    // Adds a manifest to the index.
    HRESULT AppInstallerSQLiteIndexAddManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING relativePath);

    // Updates a manifest in the index.
    HRESULT AppInstallerSQLiteIndexUpdateManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING oldManifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING oldRelativePath, 
        APPINSTALLER_SQLITE_INDEX_STRING newManifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING newRelativePath);

    // Removes a manifest from the index.
    HRESULT AppInstallerSQLiteIndexRemoveManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING relativePath);
}
