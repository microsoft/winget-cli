// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "AddPackageCatalogOptions.h"
#pragma warning( pop )
#include "AddPackageCatalogOptions.g.cpp"
#include "Converters.h"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring AddPackageCatalogOptions::Name()
    {
        return hstring(m_name);
    }
    void AddPackageCatalogOptions::Name(hstring const& value)
    {
        m_name = value;
    }
    hstring AddPackageCatalogOptions::SourceUri()
    {
        return hstring(m_sourceUri);
    }
    void AddPackageCatalogOptions::SourceUri(hstring const& value)
    {
        m_sourceUri = value;
    }
    hstring AddPackageCatalogOptions::Type()
    {
        return hstring(m_type);
    }
    void AddPackageCatalogOptions::Type(hstring const& value)
    {
        m_type = value;
    }
    winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel AddPackageCatalogOptions::TrustLevel()
    {
        return m_trustLevel;
    }
    void AddPackageCatalogOptions::TrustLevel(winrt::Microsoft::Management::Deployment::PackageCatalogTrustLevel const& value)
    {
        m_trustLevel = value;
    }
    hstring AddPackageCatalogOptions::CustomHeader()
    {
        return hstring(m_customHeader);
    }
    void AddPackageCatalogOptions::CustomHeader(hstring const& value)
    {
        m_customHeader = value;
    }
    bool AddPackageCatalogOptions::Explicit()
    {
        return m_explicit;
    }
    void AddPackageCatalogOptions::Explicit(bool const& value)
    {
        m_explicit = value;
    }
    CoCreatableMicrosoftManagementDeploymentClass(AddPackageCatalogOptions);
}
