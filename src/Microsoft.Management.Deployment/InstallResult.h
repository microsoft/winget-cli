#pragma once
#include "InstallResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("1b6bd7ab-725f-4b18-8747-f2d7c7dbf702")]
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;
        InstallResult(hstring const& correlationData, bool rebootRequired);
        void Initialize(hstring const& correlationData, bool rebootRequired);

        hstring CorrelationData();
        bool RebootRequired();
    private:
        std::wstring m_correlationData = L"";
        bool m_rebootRequired = false;
    };
}
