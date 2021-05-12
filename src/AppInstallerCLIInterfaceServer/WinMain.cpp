#include "pch.h"
#include <winrt/Microsoft.Management.Deployment.h>
#include <iomanip>
#include <iostream>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <wrl/module.h>
#include <wil/resource.h>
//#include "../Microsoft.Management.Deployment/Microsoft.Management.Deployment.h"

using namespace winrt;
using namespace winrt::Microsoft::Management::Deployment;

const CLSID CLSID_AppInstaller = { 0xC53A4F16, 0x787E, 0x42A4, 0xB3, 0x04, 0x29, 0xEF, 0xFB, 0x4B, 0xF5, 0x97 };  //C53A4F16-787E-42A4-B304-29EFFB4BF597
const CLSID CLSID_InstallOptions = { 0x1095f097, 0xEB96, 0x453B, 0xB4, 0xE6, 0x16, 0x13, 0x63, 0x7F, 0x3B, 0x14 };  //1095F097-EB96-453B-B4E6-1613637F3B14
const CLSID CLSID_FindPackagesOptions = { 0x572DED96, 0x9C60, 0x4526, { 0x8F, 0x92, 0xEE, 0x7D, 0x91, 0xD3, 0x8C, 0x1A } }; //572DED96-9C60-4526-8F92-EE7D91D38C1A
const CLSID CLSID_PackageMatchFilter = { 0xD02C9DAF, 0x99DC, 0x429C, { 0xB5, 0x03, 0x4E, 0x50, 0x4E, 0x4A, 0xB0, 0x00 } }; //D02C9DAF-99DC-429C-B503-4E504E4AB000
const CLSID CLSID_GetCompositeAppCatalogOptions = { 0x526534B8, 0x7E46, 0x47C8, { 0x84, 0x16, 0xB1, 0x68, 0x5C, 0x32, 0x7D, 0x37 } }; //526534B8-7E46-47C8-8416-B1685C327D37

const CLSID CLSID_AppCatalog = { 0xCecc20f03, 0x1173, 0x4d10, 0xa7, 0xa2, 0x22, 0x92, 0x20, 0x49, 0x23, 0x70 };  //Cecc20f03-1173-4d10-a7a2-229220492370


HANDLE g_hEvent;

/*CoCreatableClassWrlCreatorMapInclude(AppInstaller);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions);
CoCreatableClassWrlCreatorMapInclude(InstallOptions);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter);
CoCreatableClassWrlCreatorMapInclude(GetCompositeAppCatalogOptions);*/

CoCreatableClassWrlCreatorMapInclude(AppCatalog);
CoCreatableClassWrlCreatorMapInclude(AppCatalogInfo);
CoCreatableClassWrlCreatorMapInclude(AppInstaller);
CoCreatableClassWrlCreatorMapInclude(CatalogPackage);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions);
CoCreatableClassWrlCreatorMapInclude(FindPackagesResult);
CoCreatableClassWrlCreatorMapInclude(GetCompositeAppCatalogOptions);
CoCreatableClassWrlCreatorMapInclude(InstallOptions);
CoCreatableClassWrlCreatorMapInclude(InstallResult);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter);
CoCreatableClassWrlCreatorMapInclude(PackageVersionId);
CoCreatableClassWrlCreatorMapInclude(PackageVersionInfo);
CoCreatableClassWrlCreatorMapInclude(ResultMatch);
CoCreatableClassWrlCreatorMapInclude(Vectors);
/*
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
    ~AppInstallerClassFactory()
    {
        //if (CoReleaseServerProcess() == 0)
        AllowShutdown();
    }
    HRESULT __stdcall LockServer(BOOL lock) noexcept final
    {
        if (lock)
        {
            CoAddRefServerProcess();
        }
        else 
        {
            if (0 == CoReleaseServerProcess())
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
            if (!CoReleaseServerProcess())
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

    winrt::com_ptr<IGlobalOptions> globalOptions;
    winrt::check_hresult(CoCreateInstance(CLSID_GlobalOptions, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&globalOptions)));
    winrt::check_hresult(globalOptions->Set(COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN));// | COMGLB_ENABLE_AGILE_OOP_PROXIES));
    winrt::check_hresult(globalOptions->Set(COMGLB_UNMARSHALING_POLICY, COMGLB_UNMARSHALING_POLICY_STRONG));

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

    //wil::unique_event_nothrow shutdownEvent;
    //winrt::check_hresult(shutdownEvent.create(wil::EventOptions::ManualReset));
    //winrt::check_hresult(::CoRegisterServerShutdownDelay(shutdownEvent.get(), 5 * 60 * 1000));

    //WaitForSingleObject(shutdownEvent.get(), INFINITE);
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

/*void* __stdcall winrt_get_activation_factory(std::wstring_view const& name);

struct AppInstallerActivationFactory : implements<AppInstallerActivationFactory, Windows::Foundation::IGetActivationFactory>
{
    auto GetActivationFactory(hstring const& classId)
    {

        //factory = nullptr;
        //ComPtr<IActivationFactory> activationFactory;
        auto& module = ::Microsoft::WRL::Module<::Microsoft::WRL::OutOfProc>::Create();
        //HRESULT hr = module.GetActivationFactory(classId, &activationFactory);
        module.RegisterObjects();
        //if (SUCCEEDED(hr))
        //{
            //*factory = activationFactory.Detach();
        //}
        //return hr;
        /*IInspectable factory(winrt_get_activation_factory(classId), take_ownership_from_abi);

        if (!factory)
        {
            throw hresult_class_not_available(classId);
        }

        return factory;
    }
};*/


template<int RegClsType>
class DefaultOutOfProcModuleWithRegistrationFlag;

template<int RegClsType, typename ModuleT = DefaultOutOfProcModuleWithRegistrationFlag<RegClsType>>
class OutOfProcModuleWithRegistrationFlag : public ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc, ModuleT>
{
    using Elsewhere = ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc, ModuleT>;
    using Super = ::Microsoft::WRL::Details::OutOfProcModuleBase<ModuleT>;

public:
    STDMETHOD(RegisterCOMObject)
        (_In_opt_z_ const wchar_t* serverName, _In_reads_(count) IID* clsids, _In_reads_(count) IClassFactory** factories, _Inout_updates_(count) DWORD* cookies, unsigned int count)
    {
        return ::Microsoft::WRL::Details::RegisterCOMObject<RegClsType>(serverName, clsids, factories, cookies, count);
    }
};

template<int RegClsType>
class DefaultOutOfProcModuleWithRegistrationFlag : public OutOfProcModuleWithRegistrationFlag<RegClsType, DefaultOutOfProcModuleWithRegistrationFlag<RegClsType>>
{
};
// Holds the wwinmain open until COM tells us there are no more server connections
wil::unique_event _comServerExitEvent;

// Routine Description:
// - Called back when COM says there is nothing left for our server to do and we can tear down.
#pragma warning(suppress : 4505) // this is unused, and therefore discarded, when built inside windows
static void _releaseNotifier() noexcept
{
    _comServerExitEvent.SetEvent();
}

extern void* __stdcall Microsoft_Management_Deployment_get_activation_factory([[maybe_unused]] std::wstring_view const& name);
extern bool __stdcall Microsoft_Management_Deployment_can_unload_now();

void* __stdcall winrt_get_activation_factory(
    std::wstring_view const& name)
{
    void* factory = Microsoft_Management_Deployment_get_activation_factory(name);
    //if (!factory) factory = library2_get_activation_factory(name);
    //if (!factory) factory = library3_get_activation_factory(name);
    //if (!factory) factory = library4_get_activation_factory(name);
    return factory;
}
bool __stdcall winrt_can_unload_now() noexcept
{
    return Microsoft_Management_Deployment_can_unload_now();// &&
        //library2_can_unload_now() &&
        //library3_can_unload_now() &&
        //library4_can_unload_now();
}

RO_REGISTRATION_COOKIE g_registrationCookie;

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    init_apartment();

    // Set up OutOfProc COM server stuff in case we become one.
    // WRL Module gets going right before winmain is called, so if we don't
    // set this up appropriately... other things using WRL that aren't us
    // could get messed up by the singleton module and cause unexpected errors.
    _comServerExitEvent.create();
    auto& module = OutOfProcModuleWithRegistrationFlag<REGCLS_MULTIPLEUSE>::Create(&_releaseNotifier);
    try
    {
        // OK we have to do this here and not in another method because
        // we would either have to store the module ref above in some accessible
        // variable (which would be awful because of the gigantic template name)
        // or we would have to come up with some creativity to extract it out
        // of the singleton module base without accidentally having WRL
        // think we're recreating it (and then assert because it's already created.)
        //
        // Also this is all a problem because the decrementing count of used objects
        // in this module in WRL::Module base doesn't null check the release notifier
        // callback function in the OutOfProc variant in the 18362 SDK. So if anything
        // else uses WRL directly or indirectly, it'll crash if the refcount
        // ever hits 0.
        // It does in the 19041 SDK so this can be cleaned into its own class if
        // we ever build with 19041 or later.
        auto comScope{ wil::CoInitializeEx(COINIT_MULTITHREADED) };

        //auto appInstallerFactory = make<AppInstallerClassFactory>();

        /*module.RegisterCOMObject(
            nullptr,
            _uuidof(IAppCatalog),
            appInstallerFactory.get(),
            0,
            1);*/

        /*const wchar_t* pTempStr[] =
        {
            //RuntimeClass_Microsoft_Management_Deployment_AppCatalog,
            //RuntimeClass_Microsoft_Management_Deployment_AppCatalogInfo,
            RuntimeClass_Microsoft_Management_Deployment_AppInstaller,
            RuntimeClass_Microsoft_Management_Deployment_CatalogPackage,
            RuntimeClass_Microsoft_Management_Deployment_FindPackagesOptions,
            RuntimeClass_Microsoft_Management_Deployment_FindPackagesResult,
            RuntimeClass_Microsoft_Management_Deployment_GetCompositeAppCatalogOptions,
            RuntimeClass_Microsoft_Management_Deployment_InstallOptions,
            RuntimeClass_Microsoft_Management_Deployment_InstallResult,
            RuntimeClass_Microsoft_Management_Deployment_PackageMatchFilter,
            RuntimeClass_Microsoft_Management_Deployment_PackageVersionId,
            RuntimeClass_Microsoft_Management_Deployment_PackageVersionInfo,
            RuntimeClass_Microsoft_Management_Deployment_ResultMatch,
            RuntimeClass_Microsoft_Management_Deployment_Vectors,
        };

        unsigned int objectCount = ARRAYSIZE(pTempStr);

        RETURN_IF_FAILED(module.RegisterWinRTObject(nullptr, pTempStr, &g_registrationCookie, objectCount));*/

        winrt::com_ptr<IGlobalOptions> globalOptions;
        winrt::check_hresult(CoCreateInstance(CLSID_GlobalOptions, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&globalOptions)));
        winrt::check_hresult(globalOptions->Set(COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN));// | COMGLB_ENABLE_AGILE_OOP_PROXIES));
        winrt::check_hresult(globalOptions->Set(COMGLB_UNMARSHALING_POLICY, COMGLB_UNMARSHALING_POLICY_STRONG));

        RETURN_IF_FAILED(module.RegisterObjects());
        _comServerExitEvent.wait();
        RETURN_IF_FAILED(module.UnregisterObjects());
    }
    CATCH_RETURN()

    /*try
    {
        Sleep(10000);
        RegisterWinGetFactory();
        //Windows::ApplicationModel::Core::CoreApplication::RunWithActivationFactories(make<AppInstallerActivationFactory>());
    }
    catch (winrt::hresult_error const& ex)
    {
        winrt::hstring message = ex.message();
        ::MessageBoxW(::GetDesktopWindow(), message.c_str(), L"AppInstaller", MB_OK);
        throw ex;
    }
    CoUninitialize();*/
}
