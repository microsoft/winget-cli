// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <WindowsPackageManager.h>

EXTERN_C BOOL WINAPI DllMain(
    HMODULE /* hModule */,
    DWORD reason,
    LPVOID /* lpReserved */)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        if (FAILED(WindowsPackageManagerInProcModuleInitialize()))
        {
            return FALSE;
        }
    }
    break;

    case DLL_PROCESS_DETACH:
    {
        WindowsPackageManagerInProcModuleTerminate();
    }
    break;

    default:
        return TRUE;
    }
    return TRUE;
}

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv)
{
    RETURN_HR(WindowsPackageManagerInProcModuleGetClassObject(rclsid, riid, ppv));
}

STDAPI DllCanUnloadNow()
{
    return WindowsPackageManagerInProcModuleTerminate() ? S_OK : S_FALSE;
}
