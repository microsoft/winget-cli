// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Minimal WinGetUtil types and function pointer declarations for use
// with runtime loading via LoadLibrary. Only the APIs used by
// IndexComparisonTool are defined here.
#pragma once

typedef void*           WINGET_SQLITE_INDEX_HANDLE;
typedef wchar_t const*  WINGET_STRING;

#define WINGET_SQLITE_INDEX_VERSION_LATEST ((UINT32)-1)

typedef HRESULT (__stdcall *PFN_WinGetSQLiteIndexCreate)(
    WINGET_STRING               filePath,
    UINT32                      majorVersion,
    UINT32                      minorVersion,
    WINGET_SQLITE_INDEX_HANDLE* index);

typedef HRESULT (__stdcall *PFN_WinGetSQLiteIndexAddManifest)(
    WINGET_SQLITE_INDEX_HANDLE  index,
    WINGET_STRING               manifestPath,
    WINGET_STRING               relativePath);

typedef HRESULT (__stdcall *PFN_WinGetSQLiteIndexPrepareForPackaging)(
    WINGET_SQLITE_INDEX_HANDLE  index);

typedef HRESULT (__stdcall *PFN_WinGetSQLiteIndexClose)(
    WINGET_SQLITE_INDEX_HANDLE  index);
