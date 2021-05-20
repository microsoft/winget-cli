#pragma once
#include "CreateCompositeAppCatalogOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("526534B8-7E46-47C8-8416-B1685C327D37")]
    struct CreateCompositeAppCatalogOptions : CreateCompositeAppCatalogOptionsT<CreateCompositeAppCatalogOptions>
    {
        CreateCompositeAppCatalogOptions() = default;

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::AppCatalog> Catalogs();
        winrt::Microsoft::Management::Deployment::AppCatalog LocalAppCatalog();
        void LocalAppCatalog(winrt::Microsoft::Management::Deployment::AppCatalog const& value);
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CompositeSearchBehavior();
        void CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value);
    private:
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::AppCatalog> m_catalogs{ winrt::single_threaded_vector<Microsoft::Management::Deployment::AppCatalog>() };
        winrt::Microsoft::Management::Deployment::AppCatalog m_localAppCatalog { nullptr };
        winrt::Microsoft::Management::Deployment::CompositeSearchBehavior m_compositeSearchBehavior = winrt::Microsoft::Management::Deployment::CompositeSearchBehavior::AllPackages;
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct CreateCompositeAppCatalogOptions : CreateCompositeAppCatalogOptionsT<CreateCompositeAppCatalogOptions, implementation::CreateCompositeAppCatalogOptions>
    {
    };
}
