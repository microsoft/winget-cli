#include "pch.h"
#include "FindPackagesResult.h"
#include "FindPackagesResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void FindPackagesResult::Initialize(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::MatchResult> matches)
    {
        m_matches = matches;
    }
    winrt::hresult FindPackagesResult::ErrorCode()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> FindPackagesResult::Matches()
    {
        return m_matches.GetView();
    }
    bool FindPackagesResult::WasLimitExceeded()
    {
        return m_wasLimitExceeded;
    }
}
