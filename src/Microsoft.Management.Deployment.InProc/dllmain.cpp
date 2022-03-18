// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <WindowsPackageManager.h>

extern "C"
{
    BOOL WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION DllMain(
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

    WINDOWS_PACKAGE_MANAGER_API DllGetClassObject(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID* ppv)
    {
        RETURN_HR(WindowsPackageManagerInProcModuleGetClassObject(rclsid, riid, ppv));
    }

    WINDOWS_PACKAGE_MANAGER_API DllCanUnloadNow()
    {
        return WindowsPackageManagerInProcModuleTerminate() ? S_OK : S_FALSE;
    }
}
