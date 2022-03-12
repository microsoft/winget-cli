// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "UninstallOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
#if USE_PROD_CLSIDS 
    [uuid("E1D9A11E-9F85-4D87-9C17-2B93143ADB8D")]
#else
    [uuid("AA2A5C04-1AD9-46C4-B74F-6B334AD7EB8C")]
#endif
    struct UninstallOptions : UninstallOptionsT<UninstallOptions>
    {
        UninstallOptions();

        winrt::Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value);
        winrt::Microsoft::Management::Deployment::PackageUninstallMode PackageUninstallMode();
        void PackageUninstallMode(winrt::Microsoft::Management::Deployment::PackageUninstallMode const& value);
        hstring LogOutputPath();
        void LogOutputPath(hstring const& value);
        hstring CorrelationData();
        void CorrelationData(hstring const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::PackageVersionId m_packageVersionId{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageUninstallMode m_packageUninstallMode = winrt::Microsoft::Management::Deployment::PackageUninstallMode::Default;
        std::wstring m_logOutputPath = L"";
        std::wstring m_correlationData = L"";
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct UninstallOptions : UninstallOptionsT<UninstallOptions, implementation::UninstallOptions>
    {
    };
}
#endif
