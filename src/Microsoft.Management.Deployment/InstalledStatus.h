// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstalledStatus.g.h"
#include <winget/RepositorySearch.h>
#include <winget/InstalledStatus.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstalledStatus : InstalledStatusT<InstalledStatus>
    {
        InstalledStatus() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ::AppInstaller::Repository::InstalledStatus& installedStatus);
#endif

        winrt::Microsoft::Management::Deployment::InstalledStatusType Type();
        hstring Path();
        winrt::hresult Status();
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::InstalledStatusType m_type = winrt::Microsoft::Management::Deployment::InstalledStatusType::None;
        hstring m_path;
        winrt::hresult m_status = S_OK;
#endif
    };
}
