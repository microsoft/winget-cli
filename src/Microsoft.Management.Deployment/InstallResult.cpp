#include "pch.h"
#include "InstallResult.h"
#include "InstallResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void InstallResult::Initialize(hstring const& correlationData, bool rebootRequired)
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
