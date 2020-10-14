// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

extern "C"
{
    // A handle to the index.
    typedef void* WINGET_SQLITE_INDEX_HANDLE;

    // A string taken in by the utility; in UTF16.
    typedef wchar_t const* const WINGET_STRING;

    // A string returned by the utility; in UTF16.
    typedef BSTR WINGET_STRING_OUT;

#define WINGET_UTIL_API HRESULT __stdcall

#define WINGET_SQLITE_INDEX_VERSION_LATEST ((UINT32)-1)

    // Initializes the logging infrastructure.
    WINGET_UTIL_API WinGetLoggingInit(
        WINGET_STRING logPath);

    // Removes the given log file from the logging infrastructure.
    // If logPath is nullptr, then remove all loggers.
    WINGET_UTIL_API WinGetLoggingTerm(
        WINGET_STRING logPath);

    // Creates a new index file at filePath with the given version.
    WINGET_UTIL_API WinGetSQLiteIndexCreate(
        WINGET_STRING filePath, 
        UINT32 majorVersion,
        UINT32 minorVersion,
        WINGET_SQLITE_INDEX_HANDLE* index);

    // Opens an existing index at filePath.
    WINGET_UTIL_API WinGetSQLiteIndexOpen(
        WINGET_STRING filePath, 
        WINGET_SQLITE_INDEX_HANDLE* index);

    // Closes the index.
    WINGET_UTIL_API WinGetSQLiteIndexClose(
        WINGET_SQLITE_INDEX_HANDLE index);

    // Adds the manifest at the repository relative path to the index.
    // If the function succeeds, the manifest has been added.
    WINGET_UTIL_API WinGetSQLiteIndexAddManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, 
        WINGET_STRING relativePath);

    // Updates the manifest with matching { Id, Version, Channel } in the index.
    // The return value indicates whether the index was modified by the function.
    WINGET_UTIL_API WinGetSQLiteIndexUpdateManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, 
        WINGET_STRING relativePath,
        BOOL* indexModified);

    // Removes the manifest with matching { Id, Version, Channel } from the index.
    // Path is currently ignored.
    WINGET_UTIL_API WinGetSQLiteIndexRemoveManifest(
        WINGET_SQLITE_INDEX_HANDLE index, 
        WINGET_STRING manifestPath, 
        WINGET_STRING relativePath);

    // Removes data that is no longer needed for an index that is to be published.
    WINGET_UTIL_API WinGetSQLiteIndexPrepareForPackaging(
        WINGET_SQLITE_INDEX_HANDLE index);

    // Checks the index for consistency, ensuring that at a minimum all referenced rows actually exist.
    WINGET_UTIL_API WinGetSQLiteIndexCheckConsistency(
        WINGET_SQLITE_INDEX_HANDLE index,
        BOOL* succeeded);

    // Validates a given manifest. Returns a bool for validation result and
    // a string representing validation errors if validation failed.
    WINGET_UTIL_API WinGetValidateManifest(
        WINGET_STRING manifestPath,
        BOOL* succeeded,
        WINGET_STRING_OUT* message);

    // Downloads a file to the given path, returning the SHA 256 hash of the file.
    WINGET_UTIL_API WinGetDownload(
        WINGET_STRING url,
        WINGET_STRING filePath,
        BYTE* sha256Hash,
        UINT32 sha256HashLength);
}
