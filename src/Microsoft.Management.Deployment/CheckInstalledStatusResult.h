// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CheckInstalledStatusResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct CheckInstalledStatusResult : CheckInstalledStatusResultT<CheckInstalledStatusResult>
    {
        CheckInstalledStatusResult() = default;
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::CheckInstalledStatusResultStatus status,
            Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageInstallerInstalledStatus> installedStatus);
#endif

        winrt::Microsoft::Management::Deployment::CheckInstalledStatusResultStatus Status();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageInstallerInstalledStatus> PackageInstalledStatus();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::CheckInstalledStatusResultStatus m_status = winrt::Microsoft::Management::Deployment::CheckInstalledStatusResultStatus::Ok;
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageInstallerInstalledStatus> m_installedStatus{
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageInstallerInstalledStatus>() };
#endif
    };
}
