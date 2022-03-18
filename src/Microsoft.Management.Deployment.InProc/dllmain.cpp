// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <WindowsPackageManager.h>

BOOL WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION DllMain(
    HMODULE hModule,
    DWORD reason,
    LPVOID /* lpReserved */)
{
    BOOLEAN success = TRUE;
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {

    }
    break;

    case DLL_PROCESS_DETACH:
    {
        if (module != NULL)
        {
            delete module;
            module = NULL;
        }
        EventUnregisterMicrosoft_Windows_AppxPackagingOM();
        TraceLoggingUnregister(AppxPackagingProvider::Provider());

        BOOLEAN isProcessExiting = (reserved != NULL);
#if defined(__APPXPACKAGING_DOWNLEVEL_WINRT__)
        Appx::Packaging::DownlevelWinrt::DllProcessDetach(isProcessExiting);
#endif
        Downlevel::DllProcessDetach(isProcessExiting);
#if !defined(APPX_DOWNLEVEL)
        Common::CommonLibraryDllProcessDetach(isProcessExiting);
#endif
    }
    break;

    default:
        break;
    }
    return success;
}
