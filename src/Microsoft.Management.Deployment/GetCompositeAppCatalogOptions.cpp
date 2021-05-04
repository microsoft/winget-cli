#include "pch.h"
#include "GetCompositeAppCatalogOptions.h"
#include "GetCompositeAppCatalogOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> GetCompositeAppCatalogOptions::Catalogs()
    {
        return m_catalogs;
    }
    Microsoft::Management::Deployment::CompositeSearchBehavior GetCompositeAppCatalogOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void GetCompositeAppCatalogOptions::CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
}
