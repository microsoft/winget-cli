#pragma once
#include "FindPackagesResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult>
    {
        FindPackagesResult() = default;
        void Initialize(Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::MatchResult> matches);

        winrt::hresult ErrorCode();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> Matches();
        bool IsTruncated();

    private:
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::MatchResult> m_matches{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::MatchResult>() };
        bool m_isTruncated = false;
    };
}
