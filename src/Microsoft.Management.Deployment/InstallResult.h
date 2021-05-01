#pragma once
#include "InstallResult.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;
        InstallResult(hstring const& sessionId, bool rebootRequired);

        hstring CorrelationId();
        bool RebootRequired();
    private:
        std::wstring m_sessionId = L"";
        bool m_rebootRequired = false;
    };
}
