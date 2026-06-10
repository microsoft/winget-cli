#pragma once

#include "TypeResolution.h"
#include "catalog.h"

#define METADATA_FILE_EXTENSION L"winmd"
#define METADATA_FILE_PATH_FORMAT L"%s%s."  METADATA_FILE_EXTENSION
#define METADATA_FILE_SEARCH_FORMAT L"%s%s*."  METADATA_FILE_EXTENSION

namespace UndockedRegFreeWinRT
{
    static const UINT32 g_uiMaxTypeName = 512;
    static wil::unique_process_heap_string g_cachedProcessExeDir;
    static wil::unique_process_heap_string g_cachedProcessDllDir;

    BOOL CALLBACK GetProcessExeDirInitOnceCallback(
        _Inout_     PINIT_ONCE,
        _Inout_opt_ PVOID,
        _Out_opt_   PVOID*)
    {
        wil::unique_process_heap_string localExePath;
        HRESULT hr = wil::GetModuleFileNameW(nullptr, localExePath);
        if (FAILED_LOG(hr))
        {
            SetLastError(hr);
            return FALSE;
        }

        // Modify the retrieved string to truncate the actual exe name and leave the containing directory path. This API
        // expects a buffer size including the terminating null, so add 1 to the string length.
        hr = PathCchRemoveFileSpec(localExePath.get(), wcslen(localExePath.get()) + 1);
        if (FAILED_LOG(hr))
        {
            SetLastError(hr);
            return FALSE;
        }

        g_cachedProcessExeDir = std::move(localExePath);
        return TRUE;
    }

    // Returned string is cached globally, and should not be freed by the caller.
    HRESULT GetProcessExeDir(PCWSTR* path)
    {
        *path = nullptr;
        static INIT_ONCE ProcessExeDirInitOnce = INIT_ONCE_STATIC_INIT;

        RETURN_IF_WIN32_BOOL_FALSE(InitOnceExecuteOnce(&ProcessExeDirInitOnce, GetProcessExeDirInitOnceCallback, nullptr, nullptr));

        // The cache has been successfully populated by the InitOnce, so we can just use it directly.
        *path = g_cachedProcessExeDir.get();
        return S_OK;
    }

    BOOL CALLBACK GetProcessDllDirInitOnceCallback(
        _Inout_     PINIT_ONCE,
        _Inout_opt_ PVOID,
        _Out_opt_   PVOID*)
    {
        wil::unique_process_heap_string localDllPath;
        HMODULE hm = NULL;

        // Get handle to the module that contains this function (this one).
        if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCWSTR)&GetProcessDllDirInitOnceCallback, &hm) == 0)
        {
            // Error has already been set (GetLastError).
            return FALSE;
        }

        // Get the path for this module.
        HRESULT hr = wil::GetModuleFileNameW(hm, localDllPath);
        if (FAILED_LOG(hr))
        {
            SetLastError(hr);
            return FALSE;
        }

        // Modify the retrieved string to truncate the actual dll name and leave the containing directory path. This API
        // expects a buffer size including the terminating null, so add 1 to the string length.
        hr = PathCchRemoveFileSpec(localDllPath.get(), wcslen(localDllPath.get()) + 1);
        if (FAILED_LOG(hr))
        {
            SetLastError(hr);
            return FALSE;
        }

        g_cachedProcessDllDir = std::move(localDllPath);
        return TRUE;
    }

    // Returned string is cached globally, and should not be freed by the caller.
    HRESULT GetProcessDllDir(PCWSTR* path)
    {
        *path = nullptr;
        static INIT_ONCE ProcessDllDirInitOnce = INIT_ONCE_STATIC_INIT;

        RETURN_IF_WIN32_BOOL_FALSE(InitOnceExecuteOnce(&ProcessDllDirInitOnce, GetProcessDllDirInitOnceCallback, nullptr, nullptr));

        // The cache has been successfully populated by the InitOnce, so we can just use it directly.
        *path = g_cachedProcessDllDir.get();
        return S_OK;
    }

    HRESULT FindTypeInMetaDataFile(
        _In_ IMetaDataDispenserEx* pMetaDataDispenser,
        _In_ PCWSTR pszFullName,
        _In_ PCWSTR pszCandidateFilePath,
        _In_ TYPE_RESOLUTION_OPTIONS resolutionOptions,
        _COM_Outptr_opt_result_maybenull_ IMetaDataImport2** ppMetaDataImport,
        _Out_opt_ mdTypeDef* pmdTypeDef)
    {
        HRESULT hr = S_OK;
        Microsoft::WRL::ComPtr<IMetaDataImport2> spMetaDataImport;
        MetaDataImportersLRUCache* pMetaDataImporterCache = MetaDataImportersLRUCache::GetMetaDataImportersLRUCacheInstance();
        if (pMetaDataImporterCache != nullptr)
        {
            hr = pMetaDataImporterCache->GetMetaDataImporter(
                pMetaDataDispenser,
                pszCandidateFilePath,
                &spMetaDataImport);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if (SUCCEEDED(hr))
        {
            const size_t cFullName = wcslen(pszFullName);
            wchar_t pszRetrievedName[g_uiMaxTypeName];
            HCORENUM hEnum = nullptr;
            mdTypeDef rgTypeDefs[32];
            ULONG cTypeDefs;
            DWORD dwTypeDefProps;
            hr = RO_E_METADATA_NAME_NOT_FOUND;

            if (TRO_RESOLVE_TYPE & resolutionOptions)
            {
                hr = spMetaDataImport->FindTypeDefByName(pszFullName, mdTokenNil, &rgTypeDefs[0]);
                if (SUCCEEDED(hr))
                {
                    //  Check to confirm that the type we just found is a
                    //  winrt type.  If it is, we're good, otherwise we
                    //  want to fail with RO_E_INVALID_METADATA_FILE.
                    hr = spMetaDataImport->GetTypeDefProps(rgTypeDefs[0], nullptr, 0, nullptr, &dwTypeDefProps, nullptr);
                    if (SUCCEEDED(hr))
                    {
                        //  If we found the type but it's not a winrt type,
                        //  it's an error.
                        //
                        //  If the type is public, than the metadata file
                        //  is corrupt (all public types in a winrt file
                        //  must be tdWindowsRuntime).  If the type is
                        //  private, then we just want to report that the
                        //  type wasn't found.
                        if (!IsTdWindowsRuntime(dwTypeDefProps))
                        {
                            if (IsTdPublic(dwTypeDefProps))
                            {
                                hr = RO_E_INVALID_METADATA_FILE;
                            }
                            else
                            {
                                hr = RO_E_METADATA_NAME_NOT_FOUND;
                            }
                        }
                    }
                    else
                    {
                        hr = RO_E_INVALID_METADATA_FILE;
                    }
                    if (SUCCEEDED(hr))
                    {
                        if (pmdTypeDef != nullptr)
                        {
                            *pmdTypeDef = rgTypeDefs[0];
                        }
                        if (ppMetaDataImport != nullptr)
                        {
                            *ppMetaDataImport = spMetaDataImport.Detach();
                        }
                    }
                }
                else if (hr == CLDB_E_RECORD_NOTFOUND)
                {
                    hr = RO_E_METADATA_NAME_NOT_FOUND;
                }
            }

            if ((hr == RO_E_METADATA_NAME_NOT_FOUND) &&
                (TRO_RESOLVE_NAMESPACE & resolutionOptions))
            {
                // Check whether the name is a namespace rather than a type.
                do
                {
                    hr = spMetaDataImport->EnumTypeDefs(
                        &hEnum,
                        rgTypeDefs,
                        ARRAYSIZE(rgTypeDefs),
                        &cTypeDefs);

                    if (hr == S_OK)
                    {
                        for (UINT32 iTokenIndex = 0; iTokenIndex < cTypeDefs; ++iTokenIndex)
                        {
                            hr = spMetaDataImport->GetTypeDefProps(
                                rgTypeDefs[iTokenIndex],
                                pszRetrievedName,
                                ARRAYSIZE(pszRetrievedName),
                                nullptr,
                                &dwTypeDefProps,
                                nullptr);

                            if (FAILED(hr))
                            {
                                break;
                            }

                            hr = RO_E_METADATA_NAME_NOT_FOUND;

                            // Only consider windows runtime types when
                            // trying to determine if the name is a
                            // namespace.
                            if (IsTdWindowsRuntime(dwTypeDefProps) && (wcslen(pszRetrievedName) > cFullName))
                            {
                                if ((wcsncmp(pszRetrievedName, pszFullName, cFullName) == 0) &&
                                    (pszRetrievedName[cFullName] == L'.'))
                                {
                                    hr = RO_E_METADATA_NAME_IS_NAMESPACE;
                                    break;
                                }
                            }
                        }
                    }
                } while (hr == RO_E_METADATA_NAME_NOT_FOUND);

                // There were no more tokens to enumerate, but the type was still not found.
                if (hr == S_FALSE)
                {
                    hr = RO_E_METADATA_NAME_NOT_FOUND;
                }

                if (hEnum != nullptr)
                {
                    spMetaDataImport->CloseEnum(hEnum);
                    hEnum = nullptr;
                }
            }
        }
        return hr;
    }

    HRESULT FindTypeInDirectory(
        _In_ IMetaDataDispenserEx* pMetaDataDispenser,
        _In_ PCWSTR pszFullName,
        _In_ PCWSTR pszDirectoryPath,
        _Out_opt_ HSTRING* phstrMetaDataFilePath,
        _COM_Outptr_opt_result_maybenull_ IMetaDataImport2** ppMetaDataImport,
        _Out_opt_ mdTypeDef* pmdTypeDef)
    {
        HRESULT hr;

        wchar_t szCandidateFilePath[MAX_PATH + 1] = { 0 };
        wchar_t szCandidateFileName[MAX_PATH + 1] = { 0 };
        PWSTR pszLastDot;

        hr = StringCchCopy(szCandidateFileName, ARRAYSIZE(szCandidateFileName), pszFullName);

        if (SUCCEEDED(hr))
        {
            // To resolve type SomeNamespace.B.C, first check if SomeNamespace.B.C is a type or
            // a namespace in the metadata files in the directory in this order:
            // 1. SomeNamespace.B.C.WinMD
            // 2. SomeNamespace.B.WinMD
            // 3. SomeNamespace.WinMD
            do
            {
                pszLastDot = nullptr;

                hr = StringCchPrintfEx(
                    szCandidateFilePath,
                    ARRAYSIZE(szCandidateFilePath),
                    nullptr,
                    nullptr,
                    0,
                    METADATA_FILE_PATH_FORMAT,
                    pszDirectoryPath,
                    szCandidateFileName);

                if (SUCCEEDED(hr))
                {
                    hr = FindTypeInMetaDataFile(
                        pMetaDataDispenser,
                        pszFullName,
                        szCandidateFilePath,
                        TRO_RESOLVE_TYPE_AND_NAMESPACE,
                        ppMetaDataImport,
                        pmdTypeDef);

                    if (SUCCEEDED(hr))
                    {
                        if (phstrMetaDataFilePath != nullptr)
                        {
                            hr = WindowsCreateString(
                                szCandidateFilePath,
                                static_cast<UINT32>(wcslen(szCandidateFilePath)),
                                phstrMetaDataFilePath);
                        }
                        break;
                    }
                }

                hr = RO_E_METADATA_NAME_NOT_FOUND;
                pszLastDot = wcsrchr(szCandidateFileName, '.');

                if (pszLastDot != nullptr)
                {
                    *pszLastDot = '\0';
                }
            } while (pszLastDot != nullptr);

            // If name was not found when searching in the "upward direction", then
            // the name might be a namespace name in a down-level file.
            if (hr == RO_E_METADATA_NAME_NOT_FOUND)
            {
                wchar_t szFilePathSearchTemplate[MAX_PATH + 1] = { 0 };

                hr = StringCchPrintfEx(
                    szFilePathSearchTemplate,
                    ARRAYSIZE(szFilePathSearchTemplate),
                    nullptr,
                    nullptr,
                    0,
                    METADATA_FILE_SEARCH_FORMAT,
                    pszDirectoryPath,
                    pszFullName);

                if (SUCCEEDED(hr))
                {
                    WIN32_FIND_DATA fd;
                    HANDLE hFindFile;

                    // Search in all files in the directory whose name begin with the input string.
                    hFindFile = FindFirstFile(szFilePathSearchTemplate, &fd);

                    if (hFindFile != INVALID_HANDLE_VALUE)
                    {
                        PWSTR pszFilePathPart;
                        size_t cchRemaining;

                        do
                        {
                            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                            {
                                continue;
                            }

                            pszFilePathPart = szCandidateFilePath;
                            cchRemaining = ARRAYSIZE(szCandidateFilePath);
                            hr = StringCchCopyEx(
                                pszFilePathPart,
                                cchRemaining,
                                pszDirectoryPath,
                                &pszFilePathPart,
                                &cchRemaining,
                                0);

                            if (SUCCEEDED(hr))
                            {
                                hr = StringCchCopyEx(
                                    pszFilePathPart,
                                    cchRemaining,
                                    fd.cFileName,
                                    &pszFilePathPart,
                                    &cchRemaining,
                                    0);
                            }

                            if (SUCCEEDED(hr))
                            {
                                hr = FindTypeInMetaDataFile(
                                    pMetaDataDispenser,
                                    pszFullName,
                                    szCandidateFilePath,
                                    TRO_RESOLVE_NAMESPACE,
                                    ppMetaDataImport,
                                    pmdTypeDef);

                                if (hr == S_OK)
                                {
                                    hr = E_UNEXPECTED;
                                    break;
                                }

                                if (hr == RO_E_METADATA_NAME_IS_NAMESPACE)
                                {
                                    break;
                                }
                            }
                        } while (FindNextFile(hFindFile, &fd));

                        FindClose(hFindFile);
                    }
                    else
                    {
                        hr = RO_E_METADATA_NAME_NOT_FOUND;
                    }
                }
            }
        }

        if (hr == STRSAFE_E_INSUFFICIENT_BUFFER)
        {
            hr = RO_E_METADATA_NAME_NOT_FOUND;
        }

        return hr;
    }

    HRESULT FindTypeInDirectoryWithNormalization(
        _In_ IMetaDataDispenserEx* pMetaDataDispenser,
        _In_ PCWSTR pszFullName,
        _In_ PCWSTR pszDirectoryPath,
        _Out_opt_ HSTRING* phstrMetaDataFilePath,
        _COM_Outptr_opt_result_maybenull_ IMetaDataImport2** ppMetaDataImport,
        _Out_opt_ mdTypeDef* pmdTypeDef)
    {
        wchar_t pszPackagePath[MAX_PATH + 1];
        PWSTR pszPackagePathWritePtr = pszPackagePath;
        size_t cchPackagePathRemaining = ARRAYSIZE(pszPackagePath);

        HRESULT hr = StringCchCopyEx(
            pszPackagePath,
            ARRAYSIZE(pszPackagePath),
            pszDirectoryPath,
            &pszPackagePathWritePtr,
            &cchPackagePathRemaining,
            0);

        if (SUCCEEDED(hr))
        {
            // If the path is not terminated by a backslash, then append one.
            if (pszPackagePath[ARRAYSIZE(pszPackagePath) - cchPackagePathRemaining - 1] != L'\\')
            {
                hr = StringCchCopyEx(
                    pszPackagePathWritePtr,
                    cchPackagePathRemaining,
                    L"\\",
                    &pszPackagePathWritePtr,
                    &cchPackagePathRemaining,
                    0);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = FindTypeInDirectory(
                pMetaDataDispenser,
                pszFullName,
                pszPackagePath,
                phstrMetaDataFilePath,
                ppMetaDataImport,
                pmdTypeDef);
        }

        return hr;
    }

    HRESULT ResolveThirdPartyType(
        _In_ IMetaDataDispenserEx* pMetaDataDispenser,
        _In_ PCWSTR pszFullName,
        _Out_opt_ HSTRING* phstrMetaDataFilePath,
        _COM_Outptr_opt_result_maybenull_ IMetaDataImport2** ppMetaDataImport,
        _Out_opt_ mdTypeDef* pmdTypeDef)
    {
        HRESULT hr = S_OK;
        UINT32 dwPackagesCount = 0;
        UINT32 dwBufferLength = 0;

        const UINT32 filter = PACKAGE_FILTER_HEAD | PACKAGE_FILTER_DIRECT | PACKAGE_FILTER_IS_IN_RELATED_SET;
        hr = HRESULT_FROM_WIN32(GetCurrentPackageInfo(filter, &dwBufferLength, nullptr, &dwPackagesCount));
        // Only find the type next to the exe if the it is an unpackaged app. Packaged apps can have their exe on 
        // their package graph, which will allow type resolution against adjacent WinMDs.
        if (hr == HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE))
        {
            PCWSTR exeDir = nullptr;  // Never freed; owned by process global.
            RETURN_IF_FAILED(GetProcessExeDir(&exeDir));

            hr = FindTypeInDirectoryWithNormalization(
                pMetaDataDispenser,
                pszFullName,
                exeDir,
                phstrMetaDataFilePath,
                ppMetaDataImport,
                pmdTypeDef);

            if (hr == RO_E_METADATA_NAME_NOT_FOUND)
            {
                // For compatibility purposes, if we fail to find the type in the unpackaged location, we should return
                // HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE) instead of a "not found" error. This preserves the
                // behavior that existed before unpackaged type resolution was implemented.
                hr = HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE);
            }
        }
        else
        {
            hr = RO_E_METADATA_NAME_NOT_FOUND;
        }

        // If not successful looking next to an unpackaged exe, try looking next to our DLL.
        if (FAILED(hr))
        {
            PCWSTR dllDir = nullptr;  // Never freed; owned by process global.
            RETURN_IF_FAILED(GetProcessDllDir(&dllDir));

            HRESULT dllHR = FindTypeInDirectoryWithNormalization(
                pMetaDataDispenser,
                pszFullName,
                dllDir,
                phstrMetaDataFilePath,
                ppMetaDataImport,
                pmdTypeDef);

            if (SUCCEEDED(dllHR))
            {
                hr = dllHR;
            }
        }

        return hr;
    }

    //
    // MetaDataImportersLRUCache implementation
    //
    INIT_ONCE MetaDataImportersLRUCache::s_initOnce = INIT_ONCE_STATIC_INIT;
    MetaDataImportersLRUCache* MetaDataImportersLRUCache::s_pMetaDataImportersLRUCacheInstance = nullptr;

    MetaDataImportersLRUCache* MetaDataImportersLRUCache::GetMetaDataImportersLRUCacheInstance()
    {
        BOOL fInitializationSucceeded = InitOnceExecuteOnce(
            &s_initOnce,
            ConstructLRUCacheIfNecessary,
            nullptr,
            nullptr);

        UNREFERENCED_PARAMETER(fInitializationSucceeded);

        return s_pMetaDataImportersLRUCacheInstance;
    }

    // Called via InitOnceExecuteOnce.
    BOOL CALLBACK MetaDataImportersLRUCache::ConstructLRUCacheIfNecessary(
        PINIT_ONCE /*initOnce*/,
        PVOID /*parameter*/,
        PVOID* /*context*/)
    {
        HRESULT hr = S_OK;

        if (s_pMetaDataImportersLRUCacheInstance == nullptr)
        {
            s_pMetaDataImportersLRUCacheInstance = new MetaDataImportersLRUCache();

            if (s_pMetaDataImportersLRUCacheInstance == nullptr)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        return SUCCEEDED(hr);
    }

    HRESULT MetaDataImportersLRUCache::GetMetaDataImporter(
        _In_ IMetaDataDispenserEx* pMetaDataDispenser,
        _In_ PCWSTR pszCandidateFilePath,
        _Outptr_opt_ IMetaDataImport2** ppMetaDataImporter)
    {
        if (ppMetaDataImporter == nullptr)
        {
            return ERROR_BAD_ARGUMENTS;
        }

        HRESULT hr = S_OK;
        *ppMetaDataImporter = nullptr;

        EnterCriticalSection(&_csCacheLock);

        if (IsFilePathCached(pszCandidateFilePath))
        {
            // Get metadata importer from cache.
            *ppMetaDataImporter = _metadataImportersMap[pszCandidateFilePath];
            IMetaDataImport2* value = *ppMetaDataImporter;
            if (value != nullptr)
            {
                value->AddRef();
            }
        }
        else
        {
            // Importer was not found in cache.
            hr = GetNewMetaDataImporter(
                pMetaDataDispenser,
                pszCandidateFilePath,
                ppMetaDataImporter);
        }

        LeaveCriticalSection(&_csCacheLock);

        return hr;
    }

    HRESULT MetaDataImportersLRUCache::GetNewMetaDataImporter(
        _In_ IMetaDataDispenserEx* pMetaDataDispenser,
        _In_ PCWSTR pszCandidateFilePath,
        _Outptr_opt_ IMetaDataImport2** ppMetaDataImporter)
    {
        if (ppMetaDataImporter == nullptr)
        {
            return ERROR_BAD_ARGUMENTS;
        }

        HRESULT hr;

        hr = pMetaDataDispenser->OpenScope(
            pszCandidateFilePath,
            ofReadOnly,
            IID_IMetaDataImport2,
            reinterpret_cast<IUnknown**>(ppMetaDataImporter));

        if (SUCCEEDED(hr))
        {
           _metadataImportersMap.emplace(
                pszCandidateFilePath,
                *ppMetaDataImporter);
           IMetaDataImport2* value = *ppMetaDataImporter;
           if (value != nullptr)
           {
               value->AddRef();
           }
        }

        if (SUCCEEDED(hr))
        {
            hr = AddNewFilePathToList(pszCandidateFilePath);
        }

        return hr;
    }

    HRESULT MetaDataImportersLRUCache::AddNewFilePathToList(PCWSTR pszFilePath)
    {
        HRESULT hr = RemoveLeastRecentlyUsedItemIfListIsFull();

        if (SUCCEEDED(hr))
        {
            // Make room for new element.
            for (int i = g_dwMetaDataImportersLRUCacheSize - 2; i >= 0; i--)
            {
                _arFilePaths[i + 1] = _arFilePaths[i];
            }

            _arFilePaths[0] = AllocateAndCopyString(pszFilePath);

            if (_arFilePaths[0] == nullptr)
            {
                hr = E_OUTOFMEMORY;
            }
        }

        return hr;
    }

    bool MetaDataImportersLRUCache::IsFilePathCached(PCWSTR pszFilePath)
    {
        int filePathIndex = GetFilePathIndex(pszFilePath);

        if (filePathIndex != -1)
        {
            MoveElementToFrontOfList(filePathIndex);
            return true;
        }
        else
        {
            return false;
        }
    }

    int MetaDataImportersLRUCache::GetFilePathIndex(PCWSTR pszFilePath)
    {
        int filePathIndex = -1;

        for (int i = 0; (i < g_dwMetaDataImportersLRUCacheSize) && (_arFilePaths[i] != nullptr); i++)
        {
            if (wcscmp(pszFilePath, _arFilePaths[i]) == 0)
            {
                filePathIndex = i;
                break;
            }
        }

        return filePathIndex;
    }

    void MetaDataImportersLRUCache::MoveElementToFrontOfList(int elementIndex)
    {
        PWSTR pszFoundFilePath = _arFilePaths[elementIndex];

        for (int i = elementIndex - 1; i >= 0; i--)
        {
            _arFilePaths[i + 1] = _arFilePaths[i];
        }

        _arFilePaths[0] = pszFoundFilePath;
    }

    HRESULT MetaDataImportersLRUCache::RemoveLeastRecentlyUsedItemIfListIsFull()
    {
        HRESULT hr = S_OK;
        PWSTR pszLastFilePathInList = _arFilePaths[g_dwMetaDataImportersLRUCacheSize - 1];

        if (pszLastFilePathInList != nullptr)
        {
            IMetaDataImport2* value = _metadataImportersMap[pszLastFilePathInList];
            if (value != nullptr)
            {
                value->Release();
                value = nullptr;
            }
            if (!_metadataImportersMap.erase(pszLastFilePathInList))
            {
                hr = E_UNEXPECTED;
            }

            delete[] pszLastFilePathInList;
            _arFilePaths[g_dwMetaDataImportersLRUCacheSize - 1] = nullptr;
        }

        return hr;
    }
}
