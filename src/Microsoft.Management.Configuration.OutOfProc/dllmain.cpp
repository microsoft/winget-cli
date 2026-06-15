// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Factory.h"
#include <hstring.h>

using namespace Microsoft::Management::Configuration::OutOfProc;

EXTERN_C BOOL WINAPI DllMain(
    HMODULE /* hModule */,
    DWORD reason,
    LPVOID /* lpReserved */)
{
    switch (reason)
    {
    case DLL_PROCESS_DETACH:
        Factory::Terminate();
        break;
    }

    return TRUE;
}

_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID FAR* ppv) try
{
    RETURN_HR_IF(E_POINTER, !ppv);
    *ppv = nullptr;

    winrt::Windows::Foundation::IUnknown result;

    if (Factory::IsCLSID(rclsid))
    {
        result = winrt::make<Factory>().as<winrt::Windows::Foundation::IUnknown>();
    }

    if (result)
    {
        return result.as(riid, ppv);
    }

    return REGDB_E_CLASSNOTREG;
}
CATCH_RETURN();

__control_entrypoint(DllExport)
STDAPI DllCanUnloadNow()
{
    return Factory::HasReferences() ? S_FALSE : S_OK;
}

STDAPI DllGetActivationFactory(HSTRING classId, void** factory) try
{
    RETURN_HR_IF(E_POINTER, !factory);
    *factory = nullptr;

    winrt::Windows::Foundation::IUnknown result;

    if (Factory::IsCLSID(classId))
    {
        result = winrt::make<Factory>().as<winrt::Windows::Foundation::IUnknown>();
    }

    if (result)
    {
        return result.as(winrt::guid_of<winrt::Windows::Foundation::IActivationFactory>(), factory);
    }

    return REGDB_E_CLASSNOTREG;
}
CATCH_RETURN();
