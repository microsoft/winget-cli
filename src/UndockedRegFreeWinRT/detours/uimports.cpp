//////////////////////////////////////////////////////////////////////////////
//
//  Add DLLs to a module import table (uimports.cpp of detours.lib)
//
//  Microsoft Research Detours Package, Version 4.0.1
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  Note that this file is included into creatwth.cpp one or more times
//  (once for each supported module format).
//

#if DETOURS_VERSION != 0x4c0c1   // 0xMAJORcMINORcPATCH
#error detours.h version mismatch
#endif

// UpdateImports32 aka UpdateImports64
static BOOL UPDATE_IMPORTS_XX(HANDLE hProcess,
                              HMODULE hModule,
                              __in_ecount(nDlls) LPCSTR *plpDlls,
                              DWORD nDlls)
{
    BOOL fSucceeded = FALSE;
    DWORD cbNew = 0;

    BYTE * pbNew = NULL;
    DWORD i;
    SIZE_T cbRead;
    DWORD n;

    PBYTE pbModule = (PBYTE)hModule;

    IMAGE_DOS_HEADER idh;
    ZeroMemory(&idh, sizeof(idh));
    if (!ReadProcessMemory(hProcess, pbModule, &idh, sizeof(idh), &cbRead)
        || cbRead < sizeof(idh)) {

        DETOUR_TRACE(("ReadProcessMemory(idh@%p..%p) failed: %d\n",
                      pbModule, pbModule + sizeof(idh), GetLastError()));

      finish:
        if (pbNew != NULL) {
            delete[] pbNew;
            pbNew = NULL;
        }
        return fSucceeded;
    }

    IMAGE_NT_HEADERS_XX inh;
    ZeroMemory(&inh, sizeof(inh));

    if (!ReadProcessMemory(hProcess, pbModule + idh.e_lfanew, &inh, sizeof(inh), &cbRead)
        || cbRead < sizeof(inh)) {
        DETOUR_TRACE(("ReadProcessMemory(inh@%p..%p) failed: %d\n",
                      pbModule + idh.e_lfanew,
                      pbModule + idh.e_lfanew + sizeof(inh),
                      GetLastError()));
        goto finish;
    }

    if (inh.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC_XX) {
        DETOUR_TRACE(("Wrong size image (%04x != %04x).\n",
                      inh.OptionalHeader.Magic, IMAGE_NT_OPTIONAL_HDR_MAGIC_XX));
        SetLastError(ERROR_INVALID_BLOCK);
        goto finish;
    }

    // Zero out the bound table so loader doesn't use it instead of our new table.
    inh.BOUND_DIRECTORY.VirtualAddress = 0;
    inh.BOUND_DIRECTORY.Size = 0;

    // Find the size of the mapped file.
    DWORD dwSec = idh.e_lfanew +
        FIELD_OFFSET(IMAGE_NT_HEADERS_XX, OptionalHeader) +
        inh.FileHeader.SizeOfOptionalHeader;

    for (i = 0; i < inh.FileHeader.NumberOfSections; i++) {
        IMAGE_SECTION_HEADER ish;
        ZeroMemory(&ish, sizeof(ish));

        if (!ReadProcessMemory(hProcess, pbModule + dwSec + sizeof(ish) * i, &ish,
                               sizeof(ish), &cbRead)
            || cbRead < sizeof(ish)) {

            DETOUR_TRACE(("ReadProcessMemory(ish@%p..%p) failed: %d\n",
                          pbModule + dwSec + sizeof(ish) * i,
                          pbModule + dwSec + sizeof(ish) * (i + 1),
                          GetLastError()));
            goto finish;
        }

        DETOUR_TRACE(("ish[%d] : va=%08x sr=%d\n", i, ish.VirtualAddress, ish.SizeOfRawData));

        // If the file didn't have an IAT_DIRECTORY, we assign it...
        if (inh.IAT_DIRECTORY.VirtualAddress == 0 &&
            inh.IMPORT_DIRECTORY.VirtualAddress >= ish.VirtualAddress &&
            inh.IMPORT_DIRECTORY.VirtualAddress < ish.VirtualAddress + ish.SizeOfRawData) {

            inh.IAT_DIRECTORY.VirtualAddress = ish.VirtualAddress;
            inh.IAT_DIRECTORY.Size = ish.SizeOfRawData;
        }
    }

    DETOUR_TRACE(("     Imports: %p..%p\n",
                  (DWORD_PTR)pbModule + inh.IMPORT_DIRECTORY.VirtualAddress,
                  (DWORD_PTR)pbModule + inh.IMPORT_DIRECTORY.VirtualAddress +
                  inh.IMPORT_DIRECTORY.Size));

    DWORD nOldDlls = inh.IMPORT_DIRECTORY.Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
    DWORD obRem = sizeof(IMAGE_IMPORT_DESCRIPTOR) * nDlls;
    DWORD obOld = obRem + sizeof(IMAGE_IMPORT_DESCRIPTOR) * nOldDlls;
    DWORD obTab = PadToDwordPtr(obOld);
    DWORD obDll = obTab + sizeof(DWORD_XX) * 4 * nDlls;
    DWORD obStr = obDll;
    cbNew = obStr;
    for (n = 0; n < nDlls; n++) {
        cbNew += PadToDword((DWORD)strlen(plpDlls[n]) + 1);
    }

    _Analysis_assume_(cbNew >
                      sizeof(IMAGE_IMPORT_DESCRIPTOR) * (nDlls + nOldDlls)
                      + sizeof(DWORD_XX) * 4 * nDlls);
    pbNew = new BYTE [cbNew];
    if (pbNew == NULL) {
        DETOUR_TRACE(("new BYTE [cbNew] failed.\n"));
        goto finish;
    }
    ZeroMemory(pbNew, cbNew);

    PBYTE pbBase = pbModule;
    PBYTE pbNext = pbBase
        + inh.OptionalHeader.BaseOfCode
        + inh.OptionalHeader.SizeOfCode
        + inh.OptionalHeader.SizeOfInitializedData
        + inh.OptionalHeader.SizeOfUninitializedData;
    if (pbBase < pbNext) {
        pbBase = pbNext;
    }
    DETOUR_TRACE(("pbBase = %p\n", pbBase));

    PBYTE pbNewIid = FindAndAllocateNearBase(hProcess, pbModule, pbBase, cbNew);
    if (pbNewIid == NULL) {
        DETOUR_TRACE(("FindAndAllocateNearBase failed.\n"));
        goto finish;
    }

    PIMAGE_IMPORT_DESCRIPTOR piid = (PIMAGE_IMPORT_DESCRIPTOR)pbNew;
    DWORD_XX *pt;

    DWORD obBase = (DWORD)(pbNewIid - pbModule);
    DWORD dwProtect = 0;

    if (inh.IMPORT_DIRECTORY.VirtualAddress != 0) {
        // Read the old import directory if it exists.
        DETOUR_TRACE(("IMPORT_DIRECTORY perms=%x\n", dwProtect));

        if (!ReadProcessMemory(hProcess,
                               pbModule + inh.IMPORT_DIRECTORY.VirtualAddress,
                               &piid[nDlls],
                               nOldDlls * sizeof(IMAGE_IMPORT_DESCRIPTOR), &cbRead)
            || cbRead < nOldDlls * sizeof(IMAGE_IMPORT_DESCRIPTOR)) {

            DETOUR_TRACE(("ReadProcessMemory(imports) failed: %d\n", GetLastError()));
            goto finish;
        }
    }

    for (n = 0; n < nDlls; n++) {
        HRESULT hrRet = StringCchCopyA((char*)pbNew + obStr, cbNew - obStr, plpDlls[n]);
        if (FAILED(hrRet)) {
            DETOUR_TRACE(("StringCchCopyA failed: %d\n", GetLastError()));
            goto finish;
        }

        // After copying the string, we patch up the size "??" bits if any.
        hrRet = ReplaceOptionalSizeA((char*)pbNew + obStr,
                                     cbNew - obStr,
                                     DETOURS_STRINGIFY(DETOURS_BITS_XX));
        if (FAILED(hrRet)) {
            DETOUR_TRACE(("ReplaceOptionalSizeA failed: %d\n", GetLastError()));
            goto finish;
        }

        DWORD nOffset = obTab + (sizeof(DWORD_XX) * (4 * n));
        piid[n].OriginalFirstThunk = obBase + nOffset;
        pt = ((DWORD_XX*)(pbNew + nOffset));
        pt[0] = IMAGE_ORDINAL_FLAG_XX + 1;
        pt[1] = 0;

        nOffset = obTab + (sizeof(DWORD_XX) * ((4 * n) + 2));
        piid[n].FirstThunk = obBase + nOffset;
        pt = ((DWORD_XX*)(pbNew + nOffset));
        pt[0] = IMAGE_ORDINAL_FLAG_XX + 1;
        pt[1] = 0;
        piid[n].TimeDateStamp = 0;
        piid[n].ForwarderChain = 0;
        piid[n].Name = obBase + obStr;

        obStr += PadToDword((DWORD)strlen(plpDlls[n]) + 1);
    }
    _Analysis_assume_(obStr <= cbNew);

#if 0
    for (i = 0; i < nDlls + nOldDlls; i++) {
        DETOUR_TRACE(("%8d. Look=%08x Time=%08x Fore=%08x Name=%08x Addr=%08x\n",
                      i,
                      piid[i].OriginalFirstThunk,
                      piid[i].TimeDateStamp,
                      piid[i].ForwarderChain,
                      piid[i].Name,
                      piid[i].FirstThunk));
        if (piid[i].OriginalFirstThunk == 0 && piid[i].FirstThunk == 0) {
            break;
        }
    }
#endif

    if (!WriteProcessMemory(hProcess, pbNewIid, pbNew, obStr, NULL)) {
        DETOUR_TRACE(("WriteProcessMemory(iid) failed: %d\n", GetLastError()));
        goto finish;
    }

    DETOUR_TRACE(("obBaseBef = %08x..%08x\n",
                  inh.IMPORT_DIRECTORY.VirtualAddress,
                  inh.IMPORT_DIRECTORY.VirtualAddress + inh.IMPORT_DIRECTORY.Size));
    DETOUR_TRACE(("obBaseAft = %08x..%08x\n", obBase, obBase + obStr));

    // If the file doesn't have an IAT_DIRECTORY, we create it...
    if (inh.IAT_DIRECTORY.VirtualAddress == 0) {
        inh.IAT_DIRECTORY.VirtualAddress = obBase;
        inh.IAT_DIRECTORY.Size = cbNew;
    }

    inh.IMPORT_DIRECTORY.VirtualAddress = obBase;
    inh.IMPORT_DIRECTORY.Size = cbNew;

    /////////////////////// Update the NT header for the new import directory.
    //
    if (!DetourVirtualProtectSameExecuteEx(hProcess, pbModule, inh.OptionalHeader.SizeOfHeaders,
                                           PAGE_EXECUTE_READWRITE, &dwProtect)) {
        DETOUR_TRACE(("VirtualProtectEx(inh) write failed: %d\n", GetLastError()));
        goto finish;
    }

    inh.OptionalHeader.CheckSum = 0;

    if (!WriteProcessMemory(hProcess, pbModule, &idh, sizeof(idh), NULL)) {
        DETOUR_TRACE(("WriteProcessMemory(idh) failed: %d\n", GetLastError()));
        goto finish;
    }
    DETOUR_TRACE(("WriteProcessMemory(idh:%p..%p)\n", pbModule, pbModule + sizeof(idh)));

    if (!WriteProcessMemory(hProcess, pbModule + idh.e_lfanew, &inh, sizeof(inh), NULL)) {
        DETOUR_TRACE(("WriteProcessMemory(inh) failed: %d\n", GetLastError()));
        goto finish;
    }
    DETOUR_TRACE(("WriteProcessMemory(inh:%p..%p)\n",
                  pbModule + idh.e_lfanew,
                  pbModule + idh.e_lfanew + sizeof(inh)));

    if (!VirtualProtectEx(hProcess, pbModule, inh.OptionalHeader.SizeOfHeaders,
                          dwProtect, &dwProtect)) {
        DETOUR_TRACE(("VirtualProtectEx(idh) restore failed: %d\n", GetLastError()));
        goto finish;
    }

    fSucceeded = TRUE;
    goto finish;
}
