// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
#include <PackageMatchFilter.h>
#include <Client.PackageMatchFilter.h>
#pragma warning( pop )
#include "PackageMatchFilter.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption PackageMatchFilter::Option()
    {
        throw hresult_not_implemented();;
    }
    void PackageMatchFilter::Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageMatchField PackageMatchFilter::Field()
    {
        throw hresult_not_implemented();
    }
    void PackageMatchFilter::Field(winrt::Microsoft::Management::Deployment::PackageMatchField const&)
    {
        throw hresult_not_implemented();
    }
    hstring PackageMatchFilter::Value()
    {
        throw hresult_not_implemented();
    }
    void PackageMatchFilter::Value(hstring const&)
    {
        throw hresult_not_implemented();
    }
}
