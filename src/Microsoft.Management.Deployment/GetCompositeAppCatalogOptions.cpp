#include "pch.h"
#include "GetCompositeAppCatalogOptions.h"
#include "GetCompositeAppCatalogOptions.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::AppCatalog> GetCompositeAppCatalogOptions::Catalogs()
    {
        return m_catalogs;
    }
    Microsoft::Management::Deployment::LocalAppCatalog GetCompositeAppCatalogOptions::LocalAppCatalog()
    {
        throw hresult_not_implemented();
    }
    void GetCompositeAppCatalogOptions::LocalAppCatalog(Microsoft::Management::Deployment::LocalAppCatalog const& value)
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::CompositeSearchBehavior GetCompositeAppCatalogOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void GetCompositeAppCatalogOptions::CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
    CoCreatableCppWinRtClass(GetCompositeAppCatalogOptions);
}
