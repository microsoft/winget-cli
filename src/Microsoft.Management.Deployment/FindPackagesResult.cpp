#include "pch.h"
#include "FindPackagesResult.h"
#include "FindPackagesResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void FindPackagesResult::Initialize(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::ResultMatch> matches)
    {
        m_matches = matches;
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::ResultMatch> FindPackagesResult::Matches()
    {
        return m_matches.GetView();
    }
    bool FindPackagesResult::IsTruncated()
    {
        return m_isTruncated;
    }
}
