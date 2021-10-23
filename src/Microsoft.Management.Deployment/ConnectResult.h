// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConnectResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ConnectResult : ConnectResultT<ConnectResult>
    {
        ConnectResult() = default;
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(winrt::Microsoft::Management::Deployment::ConnectResultStatus status, winrt::Microsoft::Management::Deployment::PackageCatalog packageCatalog);
#endif

        winrt::Microsoft::Management::Deployment::ConnectResultStatus Status();
        winrt::Microsoft::Management::Deployment::PackageCatalog PackageCatalog();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::ConnectResultStatus m_status = winrt::Microsoft::Management::Deployment::ConnectResultStatus::Ok;
        winrt::Microsoft::Management::Deployment::PackageCatalog m_packageCatalog{ nullptr };
#endif
    };
}
