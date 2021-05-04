#pragma once
#include "InstallResult.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;
        InstallResult(hstring const& correlationData, bool rebootRequired);

        hstring CorrelationData();
        bool RebootRequired();
    private:
        std::wstring m_correlationData = L"";
        bool m_rebootRequired = false;
    };
}
