#pragma once
#include "FindPackagesOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("572DED96-9C60-4526-8F92-EE7D91D38C1A")]
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions>
    {
        FindPackagesOptions() = default;

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> Filters();
        uint32_t ResultLimit();
        void ResultLimit(uint32_t value);
        Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    private:
        uint32_t m_resultLimit = 0;
        Microsoft::Management::Deployment::CompositeSearchBehavior m_compositeSearchBehavior = Microsoft::Management::Deployment::CompositeSearchBehavior::AllPackages;
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> m_filters{ winrt::single_threaded_vector<Microsoft::Management::Deployment::PackageMatchFilter>() };
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions, implementation::FindPackagesOptions>
    {
    };
}
