#include "pch.h"
#include "CreateCompositePackageCatalogOptions.h"
#include "CreateCompositePackageCatalogOptions.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageCatalog> CreateCompositePackageCatalogOptions::Catalogs()
    {
        return m_catalogs;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog CreateCompositePackageCatalogOptions::LocalPackageCatalog()
    {
        return m_localPackageCatalog;
    }
    void CreateCompositePackageCatalogOptions::LocalPackageCatalog(winrt::Microsoft::Management::Deployment::PackageCatalog const& value)
    {
        m_localPackageCatalog = value;
    }
    winrt::Microsoft::Management::Deployment::CompositeSearchBehavior CreateCompositePackageCatalogOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void CreateCompositePackageCatalogOptions::CompositeSearchBehavior(winrt::Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
    CoCreatableCppWinRtClass(CreateCompositePackageCatalogOptions);
}
