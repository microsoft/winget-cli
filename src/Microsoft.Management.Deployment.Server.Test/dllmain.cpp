// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "winrt/base.h"
#include <windows.h>
#include <string_view>

void* __stdcall Microsoft_Management_Deployment_get_activation_factory(std::wstring_view const& name);
bool __stdcall Microsoft_Management_Deployment_can_unload_now();

BOOL APIENTRY DllMain( HMODULE,
                       DWORD  ul_reason_for_call,
                       LPVOID
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

void* __stdcall winrt_get_activation_factory(
    std::wstring_view const& name)
{
    return Microsoft_Management_Deployment_get_activation_factory(name);
}

int32_t __stdcall WINRT_GetActivationFactory(void* classId, void** factory) noexcept try
{
    std::wstring_view const name{ *reinterpret_cast<winrt::hstring*>(&classId) };
    *factory = winrt_get_activation_factory(name);

    if (*factory)
    {
        return 0;
    }

    return winrt::hresult_class_not_available(name).to_abi();
}
catch (...) { return winrt::to_hresult(); }

bool __stdcall winrt_can_unload_now() noexcept
{
    return Microsoft_Management_Deployment_can_unload_now();
}

int32_t __stdcall WINRT_CanUnloadNow() noexcept
{
    return winrt_can_unload_now() ? 0 : 1;
}