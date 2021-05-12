#pragma once
#include "FindPackagesResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("64f7bfe9-ee85-4246-9f17-3e5ce401df4b")]
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult>
    {
        FindPackagesResult() = default;
        FindPackagesResult(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::ResultMatch> matches);
        void Initialize(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::ResultMatch> matches);

        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::ResultMatch> Matches();
        bool IsTruncated();

    private:
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::ResultMatch> m_matches{ winrt::single_threaded_vector<Microsoft::Management::Deployment::ResultMatch>() };
        bool m_isTruncated = false;
    };
}
