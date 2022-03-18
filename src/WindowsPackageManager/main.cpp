// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <wil/result_macros.h>
#pragma warning( push )
#pragma warning ( disable : 4324 )
#include <wrl/module.h>
#pragma warning( pop )
#include <winrt/Microsoft.Management.Deployment.h>

#include "WindowsPackageManager.h"

#include <AppInstallerCLICore.h>
#include <Public/ComClsids.h>

using namespace winrt::Microsoft::Management::Deployment;

// CreatorMap for out-of-proc com registration
CoCreatableClassWrlCreatorMapInclude(PackageManager);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions);
CoCreatableClassWrlCreatorMapInclude(CreateCompositePackageCatalogOptions);
CoCreatableClassWrlCreatorMapInclude(InstallOptions);
CoCreatableClassWrlCreatorMapInclude(UninstallOptions);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter);

namespace
{
    CLSID GetClsidFromString()

    CLSID GetRedirectedClsidFromInProcClsid(REFCLSID clsid)
    {
        using namespace winrt::Microsoft::Management::Deployment::implementation;

        if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_PackageManager))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::PackageManager);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_FindPackagesOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::FindPackagesOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_CreateCompositePackageCatalogOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::CreateCompositePackageCatalogOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_InstallOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::InstallOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_UninstallOptions))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::UninstallOptions);
        }
        else if (IsEqualCLSID(clsid, WINGET_INPROC_COM_CLSID_PackageMatchFilter))
        {
            return __uuidof(winrt::Microsoft::Management::Deployment::implementation::PackageMatchFilter);
        }
        else
        {
            return CLSID_NULL;
        }
    }

}

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

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerInProcModuleInitialize() try
    {
        ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::InProc>::Create();
        return S_OK;
    }
    CATCH_RETURN();

    bool WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION WindowsPackageManagerInProcModuleTerminate() try
    {
        return ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::InProc>::GetModule().Terminate();
    }
    CATCH_RETURN();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerInProcModuleGetClassObject(
        REFCLSID rclsid,
        REFIID riid,
        LPVOID* ppv) try
    {
        CLSID redirectedClsid = GetRedirectedClsidFromInProcClsid(rclsid);
        RETURN_HR_IF(CLASS_E_CLASSNOTAVAILABLE, IsEqualCLSID(redirectedClsid, CLSID_NULL));
        RETURN_HR(::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::InProc>::GetModule().GetClassObject(redirectedClsid, riid, ppv));
    }
    CATCH_RETURN();
}
