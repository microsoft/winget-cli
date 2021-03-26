#include "pch.h"
#include <winrt/Microsoft.Management.Deployment.h>
#include <iomanip>
#include <iostream>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Microsoft::Management::Deployment;

const CLSID CLSID_AppInstaller = { 0xC53A4F16, 0x787E, 0x42A4, 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const CLSID CLSID_InstallOptions = { 0x1095f097, 0xEB96, 0x453B, 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 };  //1095F097-EB96-453B-B4E6-1613637F3B14

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

void RegisterWinGetFactory()
{
    g_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    DWORD appInstallerRegistration{};
    DWORD installOptionsRegistration{};

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
        REGCLS_SUSPENDED | REGCLS_SINGLEUSE,
        &appInstallerRegistration));

    auto installFactory = make<InstallOptionsClassFactory>();
    winrt::check_hresult(::CoRegisterClassObject(
        CLSID_InstallOptions,
        installFactory.get(),
        CLSCTX_LOCAL_SERVER,
        REGCLS_SUSPENDED | REGCLS_SINGLEUSE,
        &installOptionsRegistration));
    winrt::check_hresult(::CoResumeClassObjects());

    while (WaitForSingleObject(g_hEvent, 30000) == WAIT_TIMEOUT)
    {
        CheckModuleCanUnload();
    }
    winrt::check_bool(::CloseHandle(g_hEvent));

    winrt::check_hresult(::CoRevokeClassObject(appInstallerRegistration));
    winrt::check_hresult(::CoRevokeClassObject(installOptionsRegistration));
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
