#include "pch.h"
#include "GetCompositeAppCatalogOptions.h"
#include "GetCompositeAppCatalogOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> GetCompositeAppCatalogOptions::Catalogs()
    {
        throw hresult_not_implemented();
    }
    void GetCompositeAppCatalogOptions::Catalogs(Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> const& value)
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::CompositeSearchBehavior GetCompositeAppCatalogOptions::CompositeSearchBehavior()
    {
        throw hresult_not_implemented();
    }
    void GetCompositeAppCatalogOptions::CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        throw hresult_not_implemented();
    }
}
