#pragma once
#include "GetCompositeAppCatalogOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct GetCompositeAppCatalogOptions : GetCompositeAppCatalogOptionsT<GetCompositeAppCatalogOptions>
    {
        GetCompositeAppCatalogOptions() = default;

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> Catalogs();
        Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    private:
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> m_catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::AppCatalog>() };
        Microsoft::Management::Deployment::CompositeSearchBehavior m_compositeSearchBehavior = Microsoft::Management::Deployment::CompositeSearchBehavior::AllPackages;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct GetCompositeAppCatalogOptions : GetCompositeAppCatalogOptionsT<GetCompositeAppCatalogOptions, implementation::GetCompositeAppCatalogOptions>
    {
    };
}
