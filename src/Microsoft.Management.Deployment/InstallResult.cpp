#include "pch.h"
#include "InstallResult.h"
#include "InstallResult.g.cpp"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    InstallResult::InstallResult(hstring const& sessionId, bool rebootRequired)
    {
        m_sessionId = sessionId;
        m_rebootRequired = rebootRequired;
    }
    hstring InstallResult::CorrelationId()
    {
        return hstring(m_sessionId);
    }
    bool InstallResult::RebootRequired()
    {
        return m_rebootRequired;
    }
}
