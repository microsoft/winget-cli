#include "pch.h"
#include "InstallResult.h"
#include "InstallResult.g.cpp"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    InstallResult::InstallResult(hstring const& correlationData, bool rebootRequired)
    {
        m_correlationData = correlationData;
        m_rebootRequired = rebootRequired;
    }
    hstring InstallResult::CorrelationData()
    {
        return hstring(m_correlationData);
    }
    bool InstallResult::RebootRequired()
    {
        return m_rebootRequired;
    }
}
