// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

extern "C"
{
#define WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION  __stdcall
#define WINDOWS_PACKAGE_MANAGER_API HRESULT WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION

    using WindowsPackageManagerServerModuleTerminationCallback = void (*)();

    // The core function to act against command line input.
    int WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION WindowsPackageManagerCLIMain(int argc, wchar_t const** argv);

    // Initializes the Windows Package Manager COM server.
    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerInitialize();

    // Creates the server module with the given termination callback.
    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerModuleCreate(WindowsPackageManagerServerModuleTerminationCallback callback);

    // Registers the server module class factories.
    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerModuleRegister();

    // Unregisters the server module class factories.
    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerModuleUnregister();

    // Creates module for in-proc COM invocation.
    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerInProcModuleInitialize();

    // Try to terminate the module for in-proc COM. Returns false if there's still active objects.
    bool WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION WindowsPackageManagerInProcModuleTerminate();

    // DllGetClassObject for in-proc COM for cpp winrt runtime classes.
    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerInProcModuleGetClassObject(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID* ppv);
}
