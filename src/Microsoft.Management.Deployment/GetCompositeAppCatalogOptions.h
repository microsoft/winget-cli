#pragma once
#include "GetCompositeAppCatalogOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("526534B8-7E46-47C8-8416-B1685C327D37")]
    struct GetCompositeAppCatalogOptions : GetCompositeAppCatalogOptionsT<GetCompositeAppCatalogOptions>
    {
        GetCompositeAppCatalogOptions() = default;

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> Catalogs();
        Microsoft::Management::Deployment::LocalAppCatalog LocalAppCatalog();
        void LocalAppCatalog(Microsoft::Management::Deployment::LocalAppCatalog const& value);
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
