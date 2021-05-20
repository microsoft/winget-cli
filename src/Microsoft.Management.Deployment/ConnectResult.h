#pragma once
#include "ConnectResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ConnectResult : ConnectResultT<ConnectResult>
    {
        ConnectResult() = default;
        void Initialize(winrt::hresult errorCode, winrt::Microsoft::Management::Deployment::AppCatalog appCatalog);

        winrt::hresult ErrorCode();
        winrt::Microsoft::Management::Deployment::AppCatalog AppCatalog(); 
    private:
        winrt::hresult m_errorCode;
        winrt::Microsoft::Management::Deployment::AppCatalog m_appCatalog{ nullptr };
    };
}
