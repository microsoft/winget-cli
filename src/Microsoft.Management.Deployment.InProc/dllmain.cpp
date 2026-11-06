// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <WindowsPackageManager.h>
#include <hstring.h>
#include <mutex>

namespace
{
    HRESULT EnsureInitialized() try
    {
        static std::once_flag initializeOnceFlag;
        std::call_once(initializeOnceFlag,
            []()
            {
                THROW_IF_FAILED(WindowsPackageManagerInProcModuleInitialize());
            });
        return S_OK;
    }
    CATCH_RETURN();
}

EXTERN_C BOOL WINAPI DllMain(
    HMODULE /* hModule */,
    DWORD /* reason */,
    LPVOID /* lpReserved */)
{
    return TRUE;
}

STDAPI DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv)
{
    RETURN_HR_IF_NULL(E_POINTER, ppv);
    *ppv = nullptr;
    RETURN_IF_FAILED(EnsureInitialized());
    RETURN_HR(WindowsPackageManagerInProcModuleGetClassObject(rclsid, riid, ppv));
}

STDAPI DllCanUnloadNow()
{
    return WindowsPackageManagerInProcModuleTerminate() ? S_OK : S_FALSE;
}

STDAPI DllGetActivationFactory(HSTRING classId, void** factory)
{
    RETURN_HR_IF_NULL(E_POINTER, factory);
    *factory = nullptr;
    RETURN_IF_FAILED(EnsureInitialized());
    return WindowsPackageManagerInProcModuleGetActivationFactory(classId, factory);
}
