#pragma once
#include "FindPackagesOptions.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions>
    {
        FindPackagesOptions() = default;

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> Filters();
        uint32_t ResultLimit();
        void ResultLimit(uint32_t value);
        Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct FindPackagesOptions : FindPackagesOptionsT<FindPackagesOptions, implementation::FindPackagesOptions>
    {
    };
}
