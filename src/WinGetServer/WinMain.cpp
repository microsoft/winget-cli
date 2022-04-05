// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma warning( push )
#pragma warning ( disable : 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <winrt/base.h>
#include <objidl.h>
#include <WindowsPackageManager.h>

// Holds the wwinmain open until COM tells us there are no more server connections
wil::unique_event _comServerExitEvent;

// Routine Description:
// - Called back when COM says there is nothing left for our server to do and we can tear down.
static void _releaseNotifier() noexcept
{
    _comServerExitEvent.SetEvent();
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

    RETURN_IF_FAILED(WindowsPackageManagerServerInitialize());

    _comServerExitEvent.create();
    RETURN_IF_FAILED(WindowsPackageManagerServerModuleCreate(&_releaseNotifier));
    try
    {
        // Register all the CoCreatableClassWrlCreatorMapInclude classes
        RETURN_IF_FAILED(WindowsPackageManagerServerModuleRegister());
        _comServerExitEvent.wait();
        RETURN_IF_FAILED(WindowsPackageManagerServerModuleUnregister());
    }
    CATCH_RETURN()

    return 0;
}
