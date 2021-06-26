// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageMatchFilter.h"
#include "PackageMatchFilter.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption PackageMatchFilter::Option()
    {
        return m_packageMatchFilter.Option();
    }
    void PackageMatchFilter::Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value)
    {
        m_packageMatchFilter.Option(value);
    }
    winrt::Microsoft::Management::Deployment::PackageMatchField PackageMatchFilter::Field()
    {
        return m_packageMatchFilter.Field();
    }
    void PackageMatchFilter::Field(winrt::Microsoft::Management::Deployment::PackageMatchField const& value)
    {
        m_packageMatchFilter.Field(value);
    }
    hstring PackageMatchFilter::Value()
    {
        return m_packageMatchFilter.Value();
    }
    void PackageMatchFilter::Value(hstring const& value)
    {
        m_packageMatchFilter.Option();
    }
}
