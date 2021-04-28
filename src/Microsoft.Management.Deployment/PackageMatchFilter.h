#pragma once
#include "PackageMatchFilter.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter>
    {
        PackageMatchFilter() = default;

        bool IsAdditive();
        void IsAdditive(bool value);
        Microsoft::Management::Deployment::MatchType Type();
        void Type(Microsoft::Management::Deployment::MatchType const& value);
        Microsoft::Management::Deployment::PackageMatchField Field();
        void Field(Microsoft::Management::Deployment::PackageMatchField const& value);
        hstring Value();
        void Value(hstring const& value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>
    {
    };
}
