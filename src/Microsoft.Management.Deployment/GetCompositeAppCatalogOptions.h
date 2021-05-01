#pragma once
#include "GetCompositeAppCatalogOptions.g.h"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct GetCompositeAppCatalogOptions : GetCompositeAppCatalogOptionsT<GetCompositeAppCatalogOptions>
    {
        GetCompositeAppCatalogOptions() = default;

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> Catalogs();
        void Catalogs(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> const& value);
        Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct GetCompositeAppCatalogOptions : GetCompositeAppCatalogOptionsT<GetCompositeAppCatalogOptions, implementation::GetCompositeAppCatalogOptions>
    {
    };
}
