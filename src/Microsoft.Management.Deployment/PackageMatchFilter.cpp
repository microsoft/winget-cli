#include "pch.h"
#include "PackageMatchFilter.h"
#include "PackageMatchFilter.g.cpp"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    bool PackageMatchFilter::IsAdditive()
    {
        return m_isAdditive;
    }
    void PackageMatchFilter::IsAdditive(bool value)
    {
        m_isAdditive = value;
    }
    Microsoft::Management::Deployment::MatchType PackageMatchFilter::Type()
    {
        return m_matchType;
    }
    void PackageMatchFilter::Type(Microsoft::Management::Deployment::MatchType const& value)
    {
        m_matchType = value;
    }
    Microsoft::Management::Deployment::PackageMatchField PackageMatchFilter::Field()
    {
        return m_matchField;
    }
    void PackageMatchFilter::Field(Microsoft::Management::Deployment::PackageMatchField const& value)
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
}
