#include "pch.h"
#include "InstallResult.h"
#include "InstallResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    InstallResult::InstallResult(hstring const& errorText, hstring const& sessionId, bool rebootRequired)
    {
        m_errorText.assign(errorText.c_str());
        m_sessionId.assign(sessionId.c_str());
        m_rebootRequired = rebootRequired;
    }
    hstring InstallResult::ErrorText()
    {
        return hstring(m_errorText);
    }
    hstring InstallResult::SessionId()
    {
        return hstring(m_sessionId);
    }
    bool InstallResult::RebootRequired()
    {
        return m_rebootRequired;
    }
}
