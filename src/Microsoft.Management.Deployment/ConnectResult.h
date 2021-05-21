#pragma once
#include "ConnectResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ConnectResult : ConnectResultT<ConnectResult>
    {
        ConnectResult() = default;
        void Initialize(winrt::hresult errorCode, winrt::Microsoft::Management::Deployment::PackageCatalog packageCatalog);

        winrt::hresult ErrorCode();
        winrt::Microsoft::Management::Deployment::PackageCatalog PackageCatalog(); 
    private:
        winrt::hresult m_errorCode;
        winrt::Microsoft::Management::Deployment::PackageCatalog m_packageCatalog{ nullptr };
    };
}
