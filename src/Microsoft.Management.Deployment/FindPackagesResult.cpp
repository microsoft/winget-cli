#include "pch.h"
#include "FindPackagesResult.h"
#include "FindPackagesResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    FindPackagesResult::FindPackagesResult(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::ResultMatch> matches)
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
