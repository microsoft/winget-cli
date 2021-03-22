#pragma once
#include "InstallResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;

        InstallResult(hstring const& errorText, hstring const& sessionId, bool rebootRequired);
        hstring ErrorText();
        hstring SessionId();
        bool RebootRequired();
    private:
        std::wstring m_errorText = L"";
        std::wstring m_sessionId = L"";
        bool m_rebootRequired = false;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallResult : InstallResultT<InstallResult, implementation::InstallResult>
    {
    };
}
