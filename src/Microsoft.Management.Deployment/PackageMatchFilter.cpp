#include "pch.h"
#include "PackageMatchFilter.h"
#include "PackageMatchFilter.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::MatchType PackageMatchFilter::Type()
    {
        return m_matchType;
    }
    void PackageMatchFilter::Type(winrt::Microsoft::Management::Deployment::MatchType const& value)
    {
        m_matchType = value;
    }
    winrt::Microsoft::Management::Deployment::PackageMatchField PackageMatchFilter::Field()
    {
        return m_matchField;
    }
    void PackageMatchFilter::Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value)
    {
        m_matchField = value;
    }
    hstring PackageMatchFilter::Value()
    {
        return hstring(m_value);
    }
    void PackageMatchFilter::Value(hstring const& value)
    {
        m_value = value;
    }
    CoCreatableCppWinRtClass(PackageMatchFilter);
}
