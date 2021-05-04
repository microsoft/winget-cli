#include "pch.h"
#include "FindPackagesOptions.h"
#include "FindPackagesOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Filters()
    {
        return m_filters;
    }
    uint32_t FindPackagesOptions::ResultLimit()
    {
        return m_resultLimit;
    }
    void FindPackagesOptions::ResultLimit(uint32_t value)
    {
        m_resultLimit = value;
    }
    Microsoft::Management::Deployment::CompositeSearchBehavior FindPackagesOptions::CompositeSearchBehavior()
    {
        return m_compositeSearchBehavior;
    }
    void FindPackagesOptions::CompositeSearchBehavior(Microsoft::Management::Deployment::CompositeSearchBehavior const& value)
    {
        m_compositeSearchBehavior = value;
    }
}
