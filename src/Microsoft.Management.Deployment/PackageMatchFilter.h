#pragma once
#include "PackageMatchFilter.g.h"

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
    private:
        std::wstring m_value = L"";
        Microsoft::Management::Deployment::PackageMatchField m_matchField = Microsoft::Management::Deployment::PackageMatchField::AppCatalogDefined;
        Microsoft::Management::Deployment::MatchType m_matchType = Microsoft::Management::Deployment::MatchType::Exact;
        bool m_isAdditive = false;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct PackageMatchFilter : PackageMatchFilterT<PackageMatchFilter, implementation::PackageMatchFilter>
    {
    };
}
