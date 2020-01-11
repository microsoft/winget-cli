// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

extern "C"
{
    // A handle to the index.
    typedef void* APPINSTALLER_SQLITE_INDEX_HANDLE;

    // A string taken in by the utility; in UTF16.
    typedef wchar_t const* const APPINSTALLER_SQLITE_INDEX_STRING;

#define APPINSTALLER_SQLITE_INDEX_API HRESULT __stdcall

#define APPINSTALLER_SQLITE_INDEX_VERSION_LATEST ((UINT32)-1)

    // Creates a new index file at filePath with the given version.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexCreate(
        APPINSTALLER_SQLITE_INDEX_STRING filePath, 
        UINT32 majorVersion,
        UINT32 minorVersion,
        APPINSTALLER_SQLITE_INDEX_HANDLE* index);

    // Opens an existing index at filePath.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexOpen(
        APPINSTALLER_SQLITE_INDEX_STRING filePath, 
        APPINSTALLER_SQLITE_INDEX_HANDLE* index);

    // Closes the index.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexClose(
        APPINSTALLER_SQLITE_INDEX_HANDLE index);

    // Adds a manifest to the index.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexAddManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING relativePath);

    // Updates a manifest in the index.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexUpdateManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING oldManifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING oldRelativePath, 
        APPINSTALLER_SQLITE_INDEX_STRING newManifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING newRelativePath);

    // Removes a manifest from the index.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexRemoveManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING relativePath);
}
