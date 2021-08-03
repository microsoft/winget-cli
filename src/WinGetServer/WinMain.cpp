// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <winrt/Microsoft.Management.Deployment.h>
#include <wrl/module.h>
#include <winget/ExperimentalFeature.h>
#include <winget/GroupPolicy.h>
#include "COMContext.h"
#include "AppInstallerRuntime.h"
#include "AppInstallerVersions.h"

using namespace winrt::Microsoft::Management::Deployment;

CoCreatableClassWrlCreatorMapInclude(PackageManager1);
CoCreatableClassWrlCreatorMapInclude(PackageManager2);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions1);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions2);
CoCreatableClassWrlCreatorMapInclude(CreateCompositePackageCatalogOptions1);
CoCreatableClassWrlCreatorMapInclude(CreateCompositePackageCatalogOptions2);
CoCreatableClassWrlCreatorMapInclude(InstallOptions1);
CoCreatableClassWrlCreatorMapInclude(InstallOptions2);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter1);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter2);

// Holds the wwinmain open until COM tells us there are no more server connections
wil::unique_event _comServerExitEvent;

// Routine Description:
// - Called back when COM says there is nothing left for our server to do and we can tear down.
static void _releaseNotifier() noexcept
{
    _comServerExitEvent.SetEvent();
}

// Check whether the packaged api is enabled and the overarching winget group policy is enabled.
bool IsServerEnabled()
{
    ::AppInstaller::Utility::Version version("10.0.22000.0");

    if (!::AppInstaller::Runtime::IsCurrentOSVersionGreaterThanOrEqual(version) &&
        !::AppInstaller::Settings::ExperimentalFeature::IsEnabled(::AppInstaller::Settings::ExperimentalFeature::Feature::PackagedAPI))
    {
        return false;
    }
    if (!::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet))
    {
        return false;
    }

    return true;
}

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    winrt::init_apartment();

    // Enable fast rundown of objects so that the server exits faster when clients go away.
    {
        winrt::com_ptr<IGlobalOptions> globalOptions;
        winrt::check_hresult(CoCreateInstance(CLSID_GlobalOptions, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&globalOptions)));
        winrt::check_hresult(globalOptions->Set(COMGLB_RO_SETTINGS, COMGLB_FAST_RUNDOWN));
    }

    AppInstaller::COMContext::SetLoggers();

    _comServerExitEvent.create();
    auto& module = ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::Create(&_releaseNotifier);
    try
    {
        if (!IsServerEnabled())
        {
            return 0;
        }

        // Register all the CoCreatableClassWrlCreatorMapInclude classes
        RETURN_IF_FAILED(module.RegisterObjects());
        _comServerExitEvent.wait();
        RETURN_IF_FAILED(module.UnregisterObjects());

    }
    CATCH_RETURN()

    return 0;
}
