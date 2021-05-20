#pragma once
#include "PackageMatchFilter.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("D02C9DAF-99DC-429C-B503-4E504E4AB000")]
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter>
    {
        PackageMatchFilter() = default;

        winrt::Microsoft::Management::Deployment::MatchType Type();
        void Type(winrt::Microsoft::Management::Deployment::MatchType const& value);
        winrt::Microsoft::Management::Deployment::PackageMatchField Field();
        void Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value);
        hstring Value();
        void Value(hstring const& value);
    private:
        std::wstring m_value = L"";
        winrt::Microsoft::Management::Deployment::PackageMatchField m_matchField = winrt::Microsoft::Management::Deployment::PackageMatchField::AppCatalogDefined;
        winrt::Microsoft::Management::Deployment::MatchType m_matchType = winrt::Microsoft::Management::Deployment::MatchType::Exact;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>
    {
    };
}
