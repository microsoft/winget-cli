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
#include <AppInstallerFileLogger.h>
#include <AppInstallerStrings.h>
#include <AppInstallerTelemetry.h>
#include <AppInstallerErrors.h>
#include <winget/GroupPolicy.h>
#include <ShutdownMonitoring.h>
#include <winget/COMStaticStorage.h>
#include <ComClsids.h>
#include <CanUnload.h>

using namespace winrt::Microsoft::Management::Deployment;

// CreatorMap for out-of-proc com registration and direct in-proc com class construction
CoCreatableClassWrlCreatorMapInclude(PackageManager);
CoCreatableClassWrlCreatorMapInclude(FindPackagesOptions);
CoCreatableClassWrlCreatorMapInclude(CreateCompositePackageCatalogOptions);
CoCreatableClassWrlCreatorMapInclude(InstallOptions);
CoCreatableClassWrlCreatorMapInclude(UninstallOptions);
CoCreatableClassWrlCreatorMapInclude(DownloadOptions);
CoCreatableClassWrlCreatorMapInclude(PackageMatchFilter);
CoCreatableClassWrlCreatorMapInclude(AuthenticationArguments);
CoCreatableClassWrlCreatorMapInclude(PackageManagerSettings);
CoCreatableClassWrlCreatorMapInclude(RepairOptions);
CoCreatableClassWrlCreatorMapInclude(AddPackageCatalogOptions);
CoCreatableClassWrlCreatorMapInclude(RemovePackageCatalogOptions);

// Shim for configuration static functions
CoCreatableClassWrlCreatorMapInclude(ConfigurationStaticFunctionsShim);

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
        AppInstaller::ShutdownMonitoring::ServerShutdownSynchronization::Initialize(callback);
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

    void WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION WindowsPackageManagerServerWilResultLoggingCallback(const wil::FailureInfo& failure) noexcept try
    {
        AppInstaller::Logging::Telemetry().LogFailure(failure);
    }
    CATCH_LOG();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerServerCreateInstance(REFCLSID rclsid, REFIID riid, void** out) try
    {
        RETURN_HR_IF_NULL(E_POINTER, out);
        ::Microsoft::WRL::ComPtr<IClassFactory> factory;
        RETURN_IF_FAILED(::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::OutOfProc>::GetModule().GetClassObject(rclsid, IID_PPV_ARGS(&factory)));
        RETURN_HR(factory->CreateInstance(nullptr, riid, out));
    }
    CATCH_RETURN();

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerInProcModuleInitialize() try
    {
        ::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::InProc>::Create();
        AppInstaller::CLI::InProcInitialize();
        return S_OK;
    }
    CATCH_RETURN();

    bool WINDOWS_PACKAGE_MANAGER_API_CALLING_CONVENTION WindowsPackageManagerInProcModuleTerminate()
    {
        try
        {
            // Check whether the caller wants us to allow unloads
            if (implementation::GetCanUnload())
            {
                // The WRL object count is used to track externally visible objects, which largely means objects created with the `wil::details::module_count_wrapper` type wrapper.
                // Configuration objects use a composition based tracking that is similar in nature (only when OOP).
                // 
                // In-proc DllCanUnloadNow should not be blocked by our internal objects, but they must be destroyed on unload or a future reload will attempt to destroy them
                // and our module may have moved.  So when we don't have any more objects that we gave to callers, remove all of our static lifetime objects and indicate
                // that we can now be unloaded.
                if (::Microsoft::WRL::Module<::Microsoft::WRL::ModuleType::InProc>::GetModule().Terminate())
                {
                    AppInstaller::WinRT::COMStaticStorageStatics::ResetAll();
                    return true;
                }
            }
        }
        catch (...) {}

        return false;
    }

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

    WINDOWS_PACKAGE_MANAGER_API WindowsPackageManagerInProcModuleGetActivationFactory(HSTRING classId, void** factory) try
    {
        RETURN_HR_IF(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, !::AppInstaller::Settings::GroupPolicies().IsEnabled(::AppInstaller::Settings::TogglePolicy::Policy::WinGet));

        return WINRT_GetActivationFactory(classId, factory);
    }
    CATCH_RETURN();
}
