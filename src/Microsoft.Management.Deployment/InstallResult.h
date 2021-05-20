#pragma once
#include "InstallResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;
        void Initialize(hstring const& correlationData, bool rebootRequired);

        hstring CorrelationData();
        bool RebootRequired();
        winrt::hresult ErrorCode();
        winrt::hresult ExtendedErrorCode();
    private:
        std::wstring m_correlationData = L"";
        bool m_rebootRequired = false;
        winrt::hresult m_errorCode = S_OK;
        winrt::hresult m_extendedErrorCode = S_OK;
    };
}
