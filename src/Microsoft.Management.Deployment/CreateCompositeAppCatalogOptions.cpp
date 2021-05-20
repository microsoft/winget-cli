#include "pch.h"
#include "CreateCompositeAppCatalogOptions.h"
#include "CreateCompositeAppCatalogOptions.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::AppCatalog> CreateCompositeAppCatalogOptions::Catalogs()
    {
        return m_catalogs;
    }
    winrt::Microsoft::Management::Deployment::AppCatalog CreateCompositeAppCatalogOptions::LocalAppCatalog()
    {
        return m_localAppCatalog;
    }
    void CreateCompositeAppCatalogOptions::LocalAppCatalog(winrt::Microsoft::Management::Deployment::AppCatalog const& value)
    {
        m_localAppCatalog = value;
    }
    winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CreateCompositeAppCatalogOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void CreateCompositeAppCatalogOptions::CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
    CoCreatableCppWinRtClass(CreateCompositeAppCatalogOptions);
}
