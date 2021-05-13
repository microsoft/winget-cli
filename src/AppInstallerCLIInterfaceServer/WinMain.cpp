#include "pch.h"
#include <winrt/Microsoft.Management.Deployment.h>
#include <wrl/module.h>
#include <wil/resource.h>

using namespace winrt::Microsoft::Management::Deployment;

CoCreatableClassWrlCreatorMapInclude(AppInstaller);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions);
CoCreatableClassWrlCreatorMapInclude(GetCompositeAppCatalogOptions);
CoCreatableClassWrlCreatorMapInclude(InstallOptions);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter);

// Holds the wwinmain open until COM tells us there are no more server connections
wil::unique_event _comServerExitEvent;

// Routine Description:
// - Called back when COM says there is nothing left for our server to do and we can tear down.
static void _releaseNotifier() noexcept
{
    _comServerExitEvent.SetEvent();
}

int __stdcall wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    winrt::init_apartment();

    {
        winrt::com_ptr<IGlobalOptions> globalOptions;
        winrt::check_hresult(CoCreateInstance(CLSID_GlobalOptions, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&globalOptions)));
        winrt::check_hresult(globalOptions->Set(COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN));
    }

    _comServerExitEvent.create();
    auto& module = ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::Create(&_releaseNotifier);
    try
    {
        RETURN_IF_FAILED(module.RegisterObjects());
        _comServerExitEvent.wait();
        RETURN_IF_FAILED(module.UnregisterObjects());
    }
    CATCH_RETURN()

    return 0;
}
