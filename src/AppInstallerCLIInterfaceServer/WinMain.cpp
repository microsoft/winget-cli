#include "pch.h"
#include <winrt/Microsoft.Management.Deployment.h>
#include <iomanip>
#include <iostream>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Microsoft::Management::Deployment;

const CLSID CLSID_AppInstaller = { 0xC53A4F16, 0x787E, 0x42A4, 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const CLSID CLSID_InstallOptions = { 0x1095f097, 0xEB96, 0x453B, 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 };  //1095F097-EB96-453B-B4E6-1613637F3B14
const CLSID CLSID_FindPackagesOptions = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A
const CLSID CLSID_PackageMatchFilter = { 0xD02C9DAF, 0x99DC, 0x429C, { 0xB5, 0x03, 0x4E, 0x50, 0x4E, 0x4A, 0xB0, 0x00 } }; //D02C9DAF-99DC-429C-B503-4E504E4AB000
const CLSID CLSID_GetCompositeAppCatalogOptions = { 0x526534B8, 0x7E46, 0x47C8, { 0x84, 0x16, 0xB1, 0x68, 0x5C, 0x32, 0x7D, 0x37 } }; //526534B8-7E46-47C8-8416-B1685C327D37

HANDLE g_hEvent;

void AllowShutdown()
{
    SetEvent(g_hEvent);
}

typedef HRESULT(__cdecl* DLLCANUNLOADNOW)();
void CheckModuleCanUnload()
{
    HINSTANCE winRTModule;
    DLLCANUNLOADNOW dllCanUnloadNow;

    winRTModule = GetModuleHandle(L"Microsoft.Management.Deployment.dll");
    if (winRTModule == NULL)
    {
        AllowShutdown();
        return;
    }

    dllCanUnloadNow = (DLLCANUNLOADNOW)GetProcAddress(winRTModule, "DllCanUnloadNow");
    if (dllCanUnloadNow != NULL)
    {
        HRESULT canUnload = dllCanUnloadNow();
        if (canUnload == S_OK)
        {
            AllowShutdown();
        }
    }
}

struct AppInstallerClassFactory : implements<AppInstallerClassFactory, IClassFactory>
{
    HRESULT __stdcall CreateInstance(
        IUnknown* outer,
        GUID const& iid,
        void** result) noexcept final
    {
        *result = nullptr;

        if (outer)
        {
            return CLASS_E_NOAGGREGATION;
        }

        HRESULT hr = S_OK;
        AppInstaller appInstaller;
        auto iUnknown = appInstaller.as<IUnknown>();
        hr = iUnknown->QueryInterface(iid, result);
        return hr;
    }

    HRESULT __stdcall LockServer(BOOL lock) noexcept final
    {
        if (lock)
        {
            CoAddRefServerProcess();
        }
        else 
        {
            if (CoReleaseServerProcess() == 0)
            {
                CheckModuleCanUnload();
            }
        }
        return S_OK;
    }
};

struct InstallOptionsClassFactory : implements<InstallOptionsClassFactory, IClassFactory>
{
    HRESULT __stdcall CreateInstance(
        IUnknown* outer,
        GUID const& iid,
        void** result) noexcept final
    {
        *result = nullptr;

        if (outer)
        {
            return CLASS_E_NOAGGREGATION;
        }

        HRESULT hr = S_OK;
        InstallOptions installOptions;
        auto iUnknown = installOptions.as<IUnknown>();
        hr = iUnknown->QueryInterface(iid, result);
        return hr;
    }

    HRESULT __stdcall LockServer(BOOL lock) noexcept final
    {
        if (lock)
        {
            CoAddRefServerProcess();
        }
        else
        {
            if (CoReleaseServerProcess() == 0)
            {
                CheckModuleCanUnload();
            }
        }
        return S_OK;
    }
};

struct FindPackagesOptionsClassFactory : implements<FindPackagesOptionsClassFactory, IClassFactory>
{
    HRESULT __stdcall CreateInstance(
        IUnknown* outer,
        GUID const& iid,
        void** result) noexcept final
    {
        *result = nullptr;

        if (outer)
        {
            return CLASS_E_NOAGGREGATION;
        }

        HRESULT hr = S_OK;
        FindPackagesOptions findPackagesOptions;
        auto iUnknown = findPackagesOptions.as<IUnknown>();
        hr = iUnknown->QueryInterface(iid, result);
        return hr;
    }

    HRESULT __stdcall LockServer(BOOL lock) noexcept final
    {
        if (lock)
        {
            CoAddRefServerProcess();
        }
        else
        {
            if (CoReleaseServerProcess() == 0)
            {
                CheckModuleCanUnload();
            }
        }
        return S_OK;
    }
};

struct PackageMatchFilterClassFactory : implements<PackageMatchFilterClassFactory, IClassFactory>
{
    HRESULT __stdcall CreateInstance(
        IUnknown* outer,
        GUID const& iid,
        void** result) noexcept final
    {
        *result = nullptr;

        if (outer)
        {
            return CLASS_E_NOAGGREGATION;
        }

        HRESULT hr = S_OK;
        PackageMatchFilter packageMatchFilter;
        auto iUnknown = packageMatchFilter.as<IUnknown>();
        hr = iUnknown->QueryInterface(iid, result);
        return hr;
    }

    HRESULT __stdcall LockServer(BOOL lock) noexcept final
    {
        if (lock)
        {
            CoAddRefServerProcess();
        }
        else
        {
            if (CoReleaseServerProcess() == 0)
            {
                CheckModuleCanUnload();
            }
        }
        return S_OK;
    }
};

struct GetCompositeAppCatalogOptionsFactory : implements<GetCompositeAppCatalogOptionsFactory, IClassFactory>
{
    HRESULT __stdcall CreateInstance(
        IUnknown* outer,
        GUID const& iid,
        void** result) noexcept final
    {
        *result = nullptr;

        if (outer)
        {
            return CLASS_E_NOAGGREGATION;
        }

        HRESULT hr = S_OK;
        GetCompositeAppCatalogOptions getCompositeAppCatalogOptions;
        auto iUnknown = getCompositeAppCatalogOptions.as<IUnknown>();
        hr = iUnknown->QueryInterface(iid, result);
        return hr;
    }

    HRESULT __stdcall LockServer(BOOL lock) noexcept final
    {
        if (lock)
        {
            CoAddRefServerProcess();
        }
        else
        {
            if (CoReleaseServerProcess() == 0)
            {
                CheckModuleCanUnload();
            }
        }
        return S_OK;
    }
};

void RegisterWinGetFactory()
{
    g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    DWORD appInstallerRegistration{};
    DWORD installOptionsRegistration{};
    DWORD findPackagesOptionsRegistration{};
    DWORD packageMatchFilterRegistration{};
    DWORD getCompositeAppCatalogOptionsRegistration{};

    winrt::check_hresult(::CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        1,
        2,
        NULL,
        EOAC_DYNAMIC_CLOAKING,
        0
    ));

    auto appInstallerFactory = make<AppInstallerClassFactory>();
    winrt::check_hresult(::CoRegisterClassObject(
        CLSID_AppInstaller,
        appInstallerFactory.get(),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SUSPENDED | REGCLS_MULTIPLEUSE,
        &appInstallerRegistration));

    auto installFactory = make<InstallOptionsClassFactory>();
    winrt::check_hresult(::CoRegisterClassObject(
        CLSID_InstallOptions,
        installFactory.get(),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SUSPENDED | REGCLS_MULTIPLEUSE,
        &installOptionsRegistration));

    auto findPackagesOptionsFactory = make<FindPackagesOptionsClassFactory>();
    winrt::check_hresult(::CoRegisterClassObject(
        CLSID_FindPackagesOptions,
        findPackagesOptionsFactory.get(),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SUSPENDED | REGCLS_MULTIPLEUSE,
        &findPackagesOptionsRegistration));

    auto packageMatchFilterFactory = make<PackageMatchFilterClassFactory>();
    winrt::check_hresult(::CoRegisterClassObject(
        CLSID_PackageMatchFilter,
        packageMatchFilterFactory.get(),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SUSPENDED | REGCLS_MULTIPLEUSE,
        &packageMatchFilterRegistration));

    auto getCompositeAppCatalogOptionsFactory = make<GetCompositeAppCatalogOptionsFactory>();
    winrt::check_hresult(::CoRegisterClassObject(
        CLSID_GetCompositeAppCatalogOptions,
        installFactory.get(),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SUSPENDED | REGCLS_MULTIPLEUSE,
        &getCompositeAppCatalogOptionsRegistration));

    winrt::check_hresult(::CoResumeClassObjects());

    while (WaitForSingleObject(g_hEvent, 30000) == WAIT_TIMEOUT)
    {
        CheckModuleCanUnload();
    }
    winrt::check_bool(::CloseHandle(g_hEvent));

    winrt::check_hresult(::CoRevokeClassObject(appInstallerRegistration));
    winrt::check_hresult(::CoRevokeClassObject(installOptionsRegistration));
    winrt::check_hresult(::CoRevokeClassObject(findPackagesOptionsRegistration));
    winrt::check_hresult(::CoRevokeClassObject(packageMatchFilterRegistration));
    winrt::check_hresult(::CoRevokeClassObject(getCompositeAppCatalogOptionsRegistration));
    return;
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment();
    try
    {
        RegisterWinGetFactory();
    }
    catch (winrt::hresult_error const& ex)
    {
        winrt::hstring message = ex.message();
        ::MessageBoxW(::GetDesktopWindow(), message.c_str(), L"AppInstaller", MB_OK);
        throw ex;
    }
    CoUninitialize();
}
