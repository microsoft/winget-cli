// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "RepairOptions.g.h"
#include "Public/ComClsids.h"
#include <winget/ModuleCountBase.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_RepairOptions)]
    struct RepairOptions : RepairOptionsT<RepairOptions>
    {
        RepairOptions();

        winrt::Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value);
        winrt::Microsoft::Management::Deployment::PackageRepairScope PackageRepairScope();
        void PackageRepairScope(winrt::Microsoft::Management::Deployment::PackageRepairScope const& value);
        winrt::Microsoft::Management::Deployment::PackageRepairMode PackageRepairMode();
        void PackageRepairMode(winrt::Microsoft::Management::Deployment::PackageRepairMode const& value);
        bool AcceptPackageAgreements();
        void AcceptPackageAgreements(bool value);
        hstring LogOutputPath();
        void LogOutputPath(hstring const& value);
        hstring CorrelationData();
        void CorrelationData(hstring const& value);
        bool AllowHashMismatch();
        void AllowHashMismatch(bool value);
        bool BypassIsStoreClientBlockedPolicyCheck();
        void BypassIsStoreClientBlockedPolicyCheck(bool value);
        bool Force();
        void Force(bool value);
        winrt::Microsoft::Management::Deployment::AuthenticationArguments AuthenticationArguments();
        void AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageVersionId m_packageVersionId{ nullptr };
        bool m_acceptPackageAgreements = false;
        bool m_allowHashMismatch = false;
        bool m_bypassIsStoreClientBlockedPolicyCheck = false;
        bool m_force = false;
        std::wstring m_logOutputPath = L"";
        std::wstring m_correlationData = L"";
        winrt::Microsoft::Management::Deployment::PackageRepairScope m_packageRepairScope = winrt::Microsoft::Management::Deployment::PackageRepairScope::Any;
        winrt::Microsoft::Management::Deployment::PackageRepairMode m_packageRepairMode = winrt::Microsoft::Management::Deployment::PackageRepairMode::Default;
        winrt::Microsoft::Management::Deployment::AuthenticationArguments m_authenticationArguments{ nullptr };
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct RepairOptions : RepairOptionsT<RepairOptions, implementation::RepairOptions>, AppInstaller::WinRT::ModuleCountBase
    {
    };
}
#endif
