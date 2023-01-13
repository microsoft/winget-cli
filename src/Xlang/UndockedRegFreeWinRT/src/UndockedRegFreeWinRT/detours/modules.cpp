//////////////////////////////////////////////////////////////////////////////
//
//  Module Enumeration Functions (modules.cpp of detours.lib)
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Module enumeration functions.
//

// #define DETOUR_DEBUG 1
#define DETOURS_INTERNAL
#include "detours.h"

#if DETOURS_VERSION != 0x4c0c1   // 0xMAJORcMINORcPATCH
#error detours.h version mismatch
#endif

#define CLR_DIRECTORY OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR]
#define IAT_DIRECTORY OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT]

//////////////////////////////////////////////////////////////////////////////
//
const GUID DETOUR_EXE_RESTORE_GUID = {
    0x2ed7a3ff, 0x3339, 0x4a8d,
    { 0x80, 0x5c, 0xd4, 0x98, 0x15, 0x3f, 0xc2, 0x8f }};

//////////////////////////////////////////////////////////////////////////////
//
PDETOUR_SYM_INFO DetourLoadImageHlp(VOID)
{
    static DETOUR_SYM_INFO symInfo;
    static PDETOUR_SYM_INFO pSymInfo = NULL;
    static BOOL failed = false;

    if (failed) {
        return NULL;
    }
    if (pSymInfo != NULL) {
        return pSymInfo;
    }

    ZeroMemory(&symInfo, sizeof(symInfo));
    // Create a real handle to the process.
#if 0
    DuplicateHandle(GetCurrentProcess(),
                    GetCurrentProcess(),
                    GetCurrentProcess(),
                    &symInfo.hProcess,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);
#else
    symInfo.hProcess = GetCurrentProcess();
#endif

    symInfo.hDbgHelp = LoadLibraryExW(L"dbghelp.dll", NULL, 0);
    if (symInfo.hDbgHelp == NULL) {
      abort:
        failed = true;
        if (symInfo.hDbgHelp != NULL) {
            FreeLibrary(symInfo.hDbgHelp);
        }
        symInfo.pfImagehlpApiVersionEx = NULL;
        symInfo.pfSymInitialize = NULL;
        symInfo.pfSymSetOptions = NULL;
        symInfo.pfSymGetOptions = NULL;
        symInfo.pfSymLoadModule64 = NULL;
        symInfo.pfSymGetModuleInfo64 = NULL;
        symInfo.pfSymFromName = NULL;
        return NULL;
    }

    symInfo.pfImagehlpApiVersionEx
        = (PF_ImagehlpApiVersionEx)GetProcAddress(symInfo.hDbgHelp,
                                                  "ImagehlpApiVersionEx");
    symInfo.pfSymInitialize
        = (PF_SymInitialize)GetProcAddress(symInfo.hDbgHelp, "SymInitialize");
    symInfo.pfSymSetOptions
        = (PF_SymSetOptions)GetProcAddress(symInfo.hDbgHelp, "SymSetOptions");
    symInfo.pfSymGetOptions
        = (PF_SymGetOptions)GetProcAddress(symInfo.hDbgHelp, "SymGetOptions");
    symInfo.pfSymLoadModule64
        = (PF_SymLoadModule64)GetProcAddress(symInfo.hDbgHelp, "SymLoadModule64");
    symInfo.pfSymGetModuleInfo64
        = (PF_SymGetModuleInfo64)GetProcAddress(symInfo.hDbgHelp, "SymGetModuleInfo64");
    symInfo.pfSymFromName
        = (PF_SymFromName)GetProcAddress(symInfo.hDbgHelp, "SymFromName");

    API_VERSION av;
    ZeroMemory(&av, sizeof(av));
    av.MajorVersion = API_VERSION_NUMBER;

    if (symInfo.pfImagehlpApiVersionEx == NULL ||
        symInfo.pfSymInitialize == NULL ||
        symInfo.pfSymLoadModule64 == NULL ||
        symInfo.pfSymGetModuleInfo64 == NULL ||
        symInfo.pfSymFromName == NULL) {
        goto abort;
    }

    symInfo.pfImagehlpApiVersionEx(&av);
    if (av.MajorVersion < API_VERSION_NUMBER) {
        goto abort;
    }

    if (!symInfo.pfSymInitialize(symInfo.hProcess, NULL, FALSE)) {
        // We won't retry the initialize if it fails.
        goto abort;
    }

    if (symInfo.pfSymGetOptions != NULL && symInfo.pfSymSetOptions != NULL) {
        DWORD dw = symInfo.pfSymGetOptions();

        dw &= ~(SYMOPT_CASE_INSENSITIVE |
                SYMOPT_UNDNAME |
                SYMOPT_DEFERRED_LOADS |
                0);
        dw |= (
#if defined(SYMOPT_EXACT_SYMBOLS)
               SYMOPT_EXACT_SYMBOLS |
#endif
#if defined(SYMOPT_NO_UNQUALIFIED_LOADS)
               SYMOPT_NO_UNQUALIFIED_LOADS |
#endif
               SYMOPT_DEFERRED_LOADS |
#if defined(SYMOPT_FAIL_CRITICAL_ERRORS)
               SYMOPT_FAIL_CRITICAL_ERRORS |
#endif
#if defined(SYMOPT_INCLUDE_32BIT_MODULES)
               SYMOPT_INCLUDE_32BIT_MODULES |
#endif
               0);
        symInfo.pfSymSetOptions(dw);
    }

    pSymInfo = &symInfo;
    return pSymInfo;
}

PVOID WINAPI DetourFindFunction(_In_ PCSTR pszModule,
                                _In_ PCSTR pszFunction)
{
    /////////////////////////////////////////////// First, try GetProcAddress.
    //
#pragma prefast(suppress:28752, "We don't do the unicode conversion for LoadLibraryExA.")
    HMODULE hModule = LoadLibraryExA(pszModule, NULL, 0);
    if (hModule == NULL) {
        return NULL;
    }

    PBYTE pbCode = (PBYTE)GetProcAddress(hModule, pszFunction);
    if (pbCode) {
        return pbCode;
    }

    ////////////////////////////////////////////////////// Then try ImageHelp.
    //
    DETOUR_TRACE(("DetourFindFunction(%hs, %hs)\n", pszModule, pszFunction));
    PDETOUR_SYM_INFO pSymInfo = DetourLoadImageHlp();
    if (pSymInfo == NULL) {
        DETOUR_TRACE(("DetourLoadImageHlp failed: %d\n",
                      GetLastError()));
        return NULL;
    }

    if (pSymInfo->pfSymLoadModule64(pSymInfo->hProcess, NULL,
                                    (PCHAR)pszModule, NULL,
                                    (DWORD64)hModule, 0) == 0) {
        if (ERROR_SUCCESS != GetLastError()) {
            DETOUR_TRACE(("SymLoadModule64(%p) failed: %d\n",
                          pSymInfo->hProcess, GetLastError()));
            return NULL;
        }
    }

    HRESULT hrRet;
    CHAR szFullName[512];
    IMAGEHLP_MODULE64 modinfo;
    ZeroMemory(&modinfo, sizeof(modinfo));
    modinfo.SizeOfStruct = sizeof(modinfo);
    if (!pSymInfo->pfSymGetModuleInfo64(pSymInfo->hProcess, (DWORD64)hModule, &modinfo)) {
        DETOUR_TRACE(("SymGetModuleInfo64(%p, %p) failed: %d\n",
                      pSymInfo->hProcess, hModule, GetLastError()));
        return NULL;
    }

    hrRet = StringCchCopyA(szFullName, sizeof(szFullName)/sizeof(CHAR), modinfo.ModuleName);
    if (FAILED(hrRet)) {
        DETOUR_TRACE(("StringCchCopyA failed: %08x\n", hrRet));
        return NULL;
    }
    hrRet = StringCchCatA(szFullName, sizeof(szFullName)/sizeof(CHAR), "!");
    if (FAILED(hrRet)) {
        DETOUR_TRACE(("StringCchCatA failed: %08x\n", hrRet));
        return NULL;
    }
    hrRet = StringCchCatA(szFullName, sizeof(szFullName)/sizeof(CHAR), pszFunction);
    if (FAILED(hrRet)) {
        DETOUR_TRACE(("StringCchCatA failed: %08x\n", hrRet));
        return NULL;
    }

    struct CFullSymbol : SYMBOL_INFO {
        CHAR szRestOfName[512];
    } symbol;
    ZeroMemory(&symbol, sizeof(symbol));
    //symbol.ModBase = (ULONG64)hModule;
    symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
#ifdef DBHLPAPI
    symbol.MaxNameLen = sizeof(symbol.szRestOfName)/sizeof(symbol.szRestOfName[0]);
#else
    symbol.MaxNameLength = sizeof(symbol.szRestOfName)/sizeof(symbol.szRestOfName[0]);
#endif

    if (!pSymInfo->pfSymFromName(pSymInfo->hProcess, szFullName, &symbol)) {
        DETOUR_TRACE(("SymFromName(%hs) failed: %d\n", szFullName, GetLastError()));
        return NULL;
    }

#if defined(DETOURS_IA64)
    // On the IA64, we get a raw code pointer from the symbol engine
    // and have to convert it to a wrapped [code pointer, global pointer].
    //
    PPLABEL_DESCRIPTOR pldEntry = (PPLABEL_DESCRIPTOR)DetourGetEntryPoint(hModule);
    PPLABEL_DESCRIPTOR pldSymbol = new PLABEL_DESCRIPTOR;

    pldSymbol->EntryPoint = symbol.Address;
    pldSymbol->GlobalPointer = pldEntry->GlobalPointer;
    return (PBYTE)pldSymbol;
#elif defined(DETOURS_ARM)
    // On the ARM, we get a raw code pointer, which we must convert into a
    // valied Thumb2 function pointer.
    return DETOURS_PBYTE_TO_PFUNC(symbol.Address);
#else
    return (PBYTE)symbol.Address;
#endif
}

//////////////////////////////////////////////////// Module Image Functions.
//

HMODULE WINAPI DetourEnumerateModules(_In_opt_ HMODULE hModuleLast)
{
    PBYTE pbLast = (PBYTE)hModuleLast + MM_ALLOCATION_GRANULARITY;

    MEMORY_BASIC_INFORMATION mbi;
    ZeroMemory(&mbi, sizeof(mbi));

    // Find the next memory region that contains a mapped PE image.
    //
    for (;; pbLast = (PBYTE)mbi.BaseAddress + mbi.RegionSize) {
        if (VirtualQuery(pbLast, &mbi, sizeof(mbi)) <= 0) {
            break;
        }

        // Skip uncommitted regions and guard pages.
        //
        if ((mbi.State != MEM_COMMIT) ||
            ((mbi.Protect & 0xff) == PAGE_NOACCESS) ||
            (mbi.Protect & PAGE_GUARD)) {
            continue;
        }

        __try {
            PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pbLast;
            if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE ||
                (DWORD)pDosHeader->e_lfanew > mbi.RegionSize ||
                (DWORD)pDosHeader->e_lfanew < sizeof(*pDosHeader)) {
                continue;
            }

            PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                              pDosHeader->e_lfanew);
            if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
                continue;
            }

            return (HMODULE)pDosHeader;
        }
#pragma prefast(suppress:28940, "A bad pointer means this probably isn't a PE header.")
        __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                 EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
            continue;
        }
    }
    return NULL;
}

PVOID WINAPI DetourGetEntryPoint(_In_opt_ HMODULE hModule)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (hModule == NULL) {
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
    }

    __try {
#pragma warning(suppress:6011) // GetModuleHandleW(NULL) never returns NULL.
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return NULL;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return NULL;
        }

        PDETOUR_CLR_HEADER pClrHeader = NULL;
        if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            if (((PIMAGE_NT_HEADERS32)pNtHeader)->CLR_DIRECTORY.VirtualAddress != 0 &&
                ((PIMAGE_NT_HEADERS32)pNtHeader)->CLR_DIRECTORY.Size != 0) {
                pClrHeader = (PDETOUR_CLR_HEADER)
                    (((PBYTE)pDosHeader)
                     + ((PIMAGE_NT_HEADERS32)pNtHeader)->CLR_DIRECTORY.VirtualAddress);
            }
        }
        else if (pNtHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            if (((PIMAGE_NT_HEADERS64)pNtHeader)->CLR_DIRECTORY.VirtualAddress != 0 &&
                ((PIMAGE_NT_HEADERS64)pNtHeader)->CLR_DIRECTORY.Size != 0) {
                pClrHeader = (PDETOUR_CLR_HEADER)
                    (((PBYTE)pDosHeader)
                     + ((PIMAGE_NT_HEADERS64)pNtHeader)->CLR_DIRECTORY.VirtualAddress);
            }
        }

        if (pClrHeader != NULL) {
            // For MSIL assemblies, we want to use the _Cor entry points.

            HMODULE hClr = GetModuleHandleW(L"MSCOREE.DLL");
            if (hClr == NULL) {
                return NULL;
            }

            SetLastError(NO_ERROR);
            return GetProcAddress(hClr, "_CorExeMain");
        }

        SetLastError(NO_ERROR);

        // Pure resource DLLs have neither an entry point nor CLR information
        // so handle them by returning NULL (LastError is NO_ERROR)
        if (pNtHeader->OptionalHeader.AddressOfEntryPoint == 0) {
            return NULL;
        }

        return ((PBYTE)pDosHeader) +
            pNtHeader->OptionalHeader.AddressOfEntryPoint;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return NULL;
    }
}

ULONG WINAPI DetourGetModuleSize(_In_opt_ HMODULE hModule)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (hModule == NULL) {
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
    }

    __try {
#pragma warning(suppress:6011) // GetModuleHandleW(NULL) never returns NULL.
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return NULL;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return NULL;
        }
        SetLastError(NO_ERROR);

        return (pNtHeader->OptionalHeader.SizeOfImage);
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return NULL;
    }
}

HMODULE WINAPI DetourGetContainingModule(_In_ PVOID pvAddr)
{
    MEMORY_BASIC_INFORMATION mbi;
    ZeroMemory(&mbi, sizeof(mbi));

    __try {
        if (VirtualQuery(pvAddr, &mbi, sizeof(mbi)) <= 0) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        // Skip uncommitted regions and guard pages.
        //
        if ((mbi.State != MEM_COMMIT) ||
            ((mbi.Protect & 0xff) == PAGE_NOACCESS) ||
            (mbi.Protect & PAGE_GUARD)) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)mbi.AllocationBase;
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return NULL;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return NULL;
        }
        SetLastError(NO_ERROR);

        return (HMODULE)pDosHeader;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_INVALID_EXE_SIGNATURE);
        return NULL;
    }
}


static inline PBYTE RvaAdjust(_Pre_notnull_ PIMAGE_DOS_HEADER pDosHeader, _In_ DWORD raddr)
{
    if (raddr != NULL) {
        return ((PBYTE)pDosHeader) + raddr;
    }
    return NULL;
}

BOOL WINAPI DetourEnumerateExports(_In_ HMODULE hModule,
                                   _In_opt_ PVOID pContext,
                                   _In_ PF_DETOUR_ENUMERATE_EXPORT_CALLBACK pfExport)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (hModule == NULL) {
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
    }

    __try {
#pragma warning(suppress:6011) // GetModuleHandleW(NULL) never returns NULL.
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return FALSE;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return FALSE;
        }

        PIMAGE_EXPORT_DIRECTORY pExportDir
            = (PIMAGE_EXPORT_DIRECTORY)
            RvaAdjust(pDosHeader,
                      pNtHeader->OptionalHeader
                      .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

        if (pExportDir == NULL) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return FALSE;
        }

        PBYTE pExportDirEnd = (PBYTE)pExportDir + pNtHeader->OptionalHeader
            .DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
        PDWORD pdwFunctions = (PDWORD)RvaAdjust(pDosHeader, pExportDir->AddressOfFunctions);
        PDWORD pdwNames = (PDWORD)RvaAdjust(pDosHeader, pExportDir->AddressOfNames);
        PWORD pwOrdinals = (PWORD)RvaAdjust(pDosHeader, pExportDir->AddressOfNameOrdinals);

        for (DWORD nFunc = 0; nFunc < pExportDir->NumberOfFunctions; nFunc++) {
            PBYTE pbCode = (pdwFunctions != NULL)
                ? (PBYTE)RvaAdjust(pDosHeader, pdwFunctions[nFunc]) : NULL;
            PCHAR pszName = NULL;

            // if the pointer is in the export region, then it is a forwarder.
            if (pbCode > (PBYTE)pExportDir && pbCode < pExportDirEnd) {
                pbCode = NULL;
            }

            for (DWORD n = 0; n < pExportDir->NumberOfNames; n++) {
                if (pwOrdinals[n] == nFunc) {
                    pszName = (pdwNames != NULL)
                        ? (PCHAR)RvaAdjust(pDosHeader, pdwNames[n]) : NULL;
                    break;
                }
            }
            ULONG nOrdinal = pExportDir->Base + nFunc;

            if (!pfExport(pContext, nOrdinal, pszName, pbCode)) {
                break;
            }
        }
        SetLastError(NO_ERROR);
        return TRUE;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return NULL;
    }
}

BOOL WINAPI DetourEnumerateImportsEx(_In_opt_ HMODULE hModule,
                                     _In_opt_ PVOID pContext,
                                     _In_opt_ PF_DETOUR_IMPORT_FILE_CALLBACK pfImportFile,
                                     _In_opt_ PF_DETOUR_IMPORT_FUNC_CALLBACK_EX pfImportFunc)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (hModule == NULL) {
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
    }

    __try {
#pragma warning(suppress:6011) // GetModuleHandleW(NULL) never returns NULL.
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return FALSE;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return FALSE;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return FALSE;
        }

        PIMAGE_IMPORT_DESCRIPTOR iidp
            = (PIMAGE_IMPORT_DESCRIPTOR)
            RvaAdjust(pDosHeader,
                      pNtHeader->OptionalHeader
                      .DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

        if (iidp == NULL) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return FALSE;
        }

        for (; iidp->OriginalFirstThunk != 0; iidp++) {

            PCSTR pszName = (PCHAR)RvaAdjust(pDosHeader, iidp->Name);
            if (pszName == NULL) {
                SetLastError(ERROR_EXE_MARKED_INVALID);
                return FALSE;
            }

            PIMAGE_THUNK_DATA pThunks = (PIMAGE_THUNK_DATA)
                RvaAdjust(pDosHeader, iidp->OriginalFirstThunk);
            PVOID * pAddrs = (PVOID *)
                RvaAdjust(pDosHeader, iidp->FirstThunk);

            HMODULE hFile = DetourGetContainingModule(pAddrs[0]);

            if (pfImportFile != NULL) {
                if (!pfImportFile(pContext, hFile, pszName)) {
                    break;
                }
            }

            DWORD nNames = 0;
            if (pThunks) {
                for (; pThunks[nNames].u1.Ordinal; nNames++) {
                    DWORD nOrdinal = 0;
                    PCSTR pszFunc = NULL;

                    if (IMAGE_SNAP_BY_ORDINAL(pThunks[nNames].u1.Ordinal)) {
                        nOrdinal = (DWORD)IMAGE_ORDINAL(pThunks[nNames].u1.Ordinal);
                    }
                    else {
                        pszFunc = (PCSTR)RvaAdjust(pDosHeader,
                                                   (DWORD)pThunks[nNames].u1.AddressOfData + 2);
                    }

                    if (pfImportFunc != NULL) {
                        if (!pfImportFunc(pContext,
                                          nOrdinal,
                                          pszFunc,
                                          &pAddrs[nNames])) {
                            break;
                        }
                    }
                }
                if (pfImportFunc != NULL) {
                    pfImportFunc(pContext, 0, NULL, NULL);
                }
            }
        }
        if (pfImportFile != NULL) {
            pfImportFile(pContext, NULL, NULL);
        }
        SetLastError(NO_ERROR);
        return TRUE;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return FALSE;
    }
}

// Context for DetourEnumerateImportsThunk, which adapts "regular" callbacks for use with "Ex".
struct _DETOUR_ENUMERATE_IMPORTS_THUNK_CONTEXT
{
    PVOID pContext;
    PF_DETOUR_IMPORT_FUNC_CALLBACK pfImportFunc;
};

// Callback for DetourEnumerateImportsEx that adapts DetourEnumerateImportsEx
// for use with a DetourEnumerateImports callback -- derefence the IAT and pass the value on.

static
BOOL
CALLBACK
DetourEnumerateImportsThunk(_In_ PVOID VoidContext,
                            _In_ DWORD nOrdinal,
                            _In_opt_ PCSTR pszFunc,
                            _In_opt_ PVOID* ppvFunc)
{
    _DETOUR_ENUMERATE_IMPORTS_THUNK_CONTEXT const * const
        pContext = (_DETOUR_ENUMERATE_IMPORTS_THUNK_CONTEXT*)VoidContext;
    return pContext->pfImportFunc(pContext->pContext, nOrdinal, pszFunc, ppvFunc ? *ppvFunc : NULL);
}

BOOL WINAPI DetourEnumerateImports(_In_opt_ HMODULE hModule,
                                   _In_opt_ PVOID pContext,
                                   _In_opt_ PF_DETOUR_IMPORT_FILE_CALLBACK pfImportFile,
                                   _In_opt_ PF_DETOUR_IMPORT_FUNC_CALLBACK pfImportFunc)
{
    _DETOUR_ENUMERATE_IMPORTS_THUNK_CONTEXT const context = { pContext, pfImportFunc };

    return DetourEnumerateImportsEx(hModule,
                                    (PVOID)&context,
                                    pfImportFile,
                                    &DetourEnumerateImportsThunk);
}

static PDETOUR_LOADED_BINARY WINAPI GetPayloadSectionFromModule(HMODULE hModule)
{
    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hModule;
    if (hModule == NULL) {
        pDosHeader = (PIMAGE_DOS_HEADER)GetModuleHandleW(NULL);
    }

    __try {
#pragma warning(suppress:6011) // GetModuleHandleW(NULL) never returns NULL.
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            SetLastError(ERROR_BAD_EXE_FORMAT);
            return NULL;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
                                                          pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return NULL;
        }
        if (pNtHeader->FileHeader.SizeOfOptionalHeader == 0) {
            SetLastError(ERROR_EXE_MARKED_INVALID);
            return NULL;
        }

        PIMAGE_SECTION_HEADER pSectionHeaders
            = (PIMAGE_SECTION_HEADER)((PBYTE)pNtHeader
                                      + sizeof(pNtHeader->Signature)
                                      + sizeof(pNtHeader->FileHeader)
                                      + pNtHeader->FileHeader.SizeOfOptionalHeader);

        for (DWORD n = 0; n < pNtHeader->FileHeader.NumberOfSections; n++) {
            if (strcmp((PCHAR)pSectionHeaders[n].Name, ".detour") == 0) {
                if (pSectionHeaders[n].VirtualAddress == 0 ||
                    pSectionHeaders[n].SizeOfRawData == 0) {

                    break;
                }

                PBYTE pbData = (PBYTE)pDosHeader + pSectionHeaders[n].VirtualAddress;
                DETOUR_SECTION_HEADER *pHeader = (DETOUR_SECTION_HEADER *)pbData;
                if (pHeader->cbHeaderSize < sizeof(DETOUR_SECTION_HEADER) ||
                    pHeader->nSignature != DETOUR_SECTION_HEADER_SIGNATURE) {

                    break;
                }

                if (pHeader->nDataOffset == 0) {
                    pHeader->nDataOffset = pHeader->cbHeaderSize;
                }
                SetLastError(NO_ERROR);
                return (PBYTE)pHeader;
            }
        }
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return NULL;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_EXE_MARKED_INVALID);
        return NULL;
    }
}

DWORD WINAPI DetourGetSizeOfPayloads(_In_opt_ HMODULE hModule)
{
    PDETOUR_LOADED_BINARY pBinary = GetPayloadSectionFromModule(hModule);
    if (pBinary == NULL) {
        // Error set by GetPayloadSectionFromModule.
        return 0;
    }

    __try {
        DETOUR_SECTION_HEADER *pHeader = (DETOUR_SECTION_HEADER *)pBinary;
        if (pHeader->cbHeaderSize < sizeof(DETOUR_SECTION_HEADER) ||
            pHeader->nSignature != DETOUR_SECTION_HEADER_SIGNATURE) {

            SetLastError(ERROR_INVALID_HANDLE);
            return 0;
        }
        SetLastError(NO_ERROR);
        return pHeader->cbDataSize;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }
}

_Writable_bytes_(*pcbData)
_Readable_bytes_(*pcbData)
_Success_(return != NULL)
PVOID WINAPI DetourFindPayload(_In_opt_ HMODULE hModule,
                               _In_ REFGUID rguid,
                               _Out_ DWORD *pcbData)
{
    PBYTE pbData = NULL;
    if (pcbData) {
        *pcbData = 0;
    }

    PDETOUR_LOADED_BINARY pBinary = GetPayloadSectionFromModule(hModule);
    if (pBinary == NULL) {
        // Error set by GetPayloadSectionFromModule.
        return NULL;
    }

    __try {
        DETOUR_SECTION_HEADER *pHeader = (DETOUR_SECTION_HEADER *)pBinary;
        if (pHeader->cbHeaderSize < sizeof(DETOUR_SECTION_HEADER) ||
            pHeader->nSignature != DETOUR_SECTION_HEADER_SIGNATURE) {

            SetLastError(ERROR_INVALID_EXE_SIGNATURE);
            return NULL;
        }

        PBYTE pbBeg = ((PBYTE)pHeader) + pHeader->nDataOffset;
        PBYTE pbEnd = ((PBYTE)pHeader) + pHeader->cbDataSize;

        for (pbData = pbBeg; pbData < pbEnd;) {
            DETOUR_SECTION_RECORD *pSection = (DETOUR_SECTION_RECORD *)pbData;

            if (pSection->guid.Data1 == rguid.Data1 &&
                pSection->guid.Data2 == rguid.Data2 &&
                pSection->guid.Data3 == rguid.Data3 &&
                pSection->guid.Data4[0] == rguid.Data4[0] &&
                pSection->guid.Data4[1] == rguid.Data4[1] &&
                pSection->guid.Data4[2] == rguid.Data4[2] &&
                pSection->guid.Data4[3] == rguid.Data4[3] &&
                pSection->guid.Data4[4] == rguid.Data4[4] &&
                pSection->guid.Data4[5] == rguid.Data4[5] &&
                pSection->guid.Data4[6] == rguid.Data4[6] &&
                pSection->guid.Data4[7] == rguid.Data4[7]) {

                if (pcbData) {
                    *pcbData = pSection->cbBytes - sizeof(*pSection);
                    SetLastError(NO_ERROR);
                    return (PBYTE)(pSection + 1);
                }
            }

            pbData = (PBYTE)pSection + pSection->cbBytes;
        }
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
    __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
             EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        SetLastError(ERROR_INVALID_HANDLE);
        return NULL;
    }
}

_Writable_bytes_(*pcbData)
_Readable_bytes_(*pcbData)
_Success_(return != NULL)
PVOID WINAPI DetourFindPayloadEx(_In_ REFGUID rguid,
                                 _Out_ DWORD * pcbData)
{
    for (HMODULE hMod = NULL; (hMod = DetourEnumerateModules(hMod)) != NULL;) {
        PVOID pvData;

        pvData = DetourFindPayload(hMod, rguid, pcbData);
        if (pvData != NULL) {
            return pvData;
        }
    }
    SetLastError(ERROR_MOD_NOT_FOUND);
    return NULL;
}

BOOL WINAPI DetourRestoreAfterWithEx(_In_reads_bytes_(cbData) PVOID pvData,
                                     _In_ DWORD cbData)
{
    PDETOUR_EXE_RESTORE pder = (PDETOUR_EXE_RESTORE)pvData;

    if (pder->cb != sizeof(*pder) || pder->cb > cbData) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return FALSE;
    }

    DWORD dwPermIdh = ~0u;
    DWORD dwPermInh = ~0u;
    DWORD dwPermClr = ~0u;
    DWORD dwIgnore;
    BOOL fSucceeded = FALSE;
    BOOL fUpdated32To64 = FALSE;

    if (pder->pclr != NULL && pder->clr.Flags != ((PDETOUR_CLR_HEADER)pder->pclr)->Flags) {
        // If we had to promote the 32/64-bit agnostic IL to 64-bit, we can't restore
        // that.
        fUpdated32To64 = TRUE;
    }

    if (DetourVirtualProtectSameExecute(pder->pidh, pder->cbidh,
                                        PAGE_EXECUTE_READWRITE, &dwPermIdh)) {
        if (DetourVirtualProtectSameExecute(pder->pinh, pder->cbinh,
                                            PAGE_EXECUTE_READWRITE, &dwPermInh)) {

            CopyMemory(pder->pidh, &pder->idh, pder->cbidh);
            CopyMemory(pder->pinh, &pder->inh, pder->cbinh);

            if (pder->pclr != NULL && !fUpdated32To64) {
                if (DetourVirtualProtectSameExecute(pder->pclr, pder->cbclr,
                                                    PAGE_EXECUTE_READWRITE, &dwPermClr)) {
                    CopyMemory(pder->pclr, &pder->clr, pder->cbclr);
                    VirtualProtect(pder->pclr, pder->cbclr, dwPermClr, &dwIgnore);
                    fSucceeded = TRUE;
                }
            }
            else {
                fSucceeded = TRUE;
            }
            VirtualProtect(pder->pinh, pder->cbinh, dwPermInh, &dwIgnore);
        }
        VirtualProtect(pder->pidh, pder->cbidh, dwPermIdh, &dwIgnore);
    }
    return fSucceeded;
}

BOOL WINAPI DetourRestoreAfterWith()
{
    PVOID pvData;
    DWORD cbData;

    pvData = DetourFindPayloadEx(DETOUR_EXE_RESTORE_GUID, &cbData);

    if (pvData != NULL && cbData != 0) {
        return DetourRestoreAfterWithEx(pvData, cbData);
    }
    SetLastError(ERROR_MOD_NOT_FOUND);
    return FALSE;
}

//  End of File
