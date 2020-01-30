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

    // Initializes the logging infrastructure.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerLoggingInit(
        APPINSTALLER_SQLITE_INDEX_STRING logPath);

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

    // Adds the manifest at the repository relative path to the index.
    // If the function succeeds, the manifest has been added.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexAddManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING relativePath);

    // Updates the manifest at the repository relative path in the index.
    // If the old manifest does not exist in the index, this is equivalent to AddManifest(newManifest, newRelativePath).
    // The out value indicates whether the index was modified by the function.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexUpdateManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING oldManifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING oldRelativePath, 
        APPINSTALLER_SQLITE_INDEX_STRING newManifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING newRelativePath,
        bool* indexModified);

    // Removes the manifest at the repository relative path from the index.
    // The out value indicates whether the index was modified by the function.
    APPINSTALLER_SQLITE_INDEX_API AppInstallerSQLiteIndexRemoveManifest(
        APPINSTALLER_SQLITE_INDEX_HANDLE index, 
        APPINSTALLER_SQLITE_INDEX_STRING manifestPath, 
        APPINSTALLER_SQLITE_INDEX_STRING relativePath,
        bool* indexModified);
}
