// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <wil/result_macros.h>
#include <wrl/module.h>
#include <winrt/Microsoft.Management.Deployment.h>

#include "WindowsPackageManager.h"

#include <AppInstallerCLICore.h>

using namespace winrt::Microsoft::Management::Deployment;

CoCreatableClassWrlCreatorMapInclude(PackageManager);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions);
CoCreatableClassWrlCreatorMapInclude(CreateCompositePackageCatalogOptions);
CoCreatableClassWrlCreatorMapInclude(InstallOptions);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter);

extern "C"
{
    int WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION WindowsPackageManagerCLIMain(int argc, wchar_t const** argv) try
    {
        ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::InProc>::Create();
        return AppInstaller::CLI::CoreMain(argc, argv);
    }
    CATCH_RETURN();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerInitialize() try
    {
        AppInstaller::CLI::ServerInitialize();
        return S_OK;
    }
    CATCH_RETURN();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerModuleCreate(WindowsPackageManagerServerModuleTerminationCallback callback) try
    {
        ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::Create(callback);
        return S_OK;
    }
    CATCH_RETURN();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerModuleRegister() try
    {
        RETURN_HR(::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::GetModule().RegisterObjects());
    }
    CATCH_RETURN();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerModuleUnregister() try
    {
        RETURN_HR(::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::GetModule().UnregisterObjects());
    }
    CATCH_RETURN();
}
