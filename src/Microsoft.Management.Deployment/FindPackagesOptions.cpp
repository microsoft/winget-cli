#include "pch.h"
#include "FindPackagesOptions.h"
#include "FindPackagesOptions.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Selectors()
    {
        return m_selectors;
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageMatchFilter> FindPackagesOptions::Filters()
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
    CoCreatableCppWinRtClass(FindPackagesOptions);
}
