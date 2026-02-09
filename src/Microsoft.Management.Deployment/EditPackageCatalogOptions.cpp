// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "EditPackageCatalogOptions.h"
#pragma warning( pop )
#include "EditPackageCatalogOptions.g.cpp"
#include "Converters.h"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring EditPackageCatalogOptions::Name()
    {
        return hstring(m_name);
    }

    void EditPackageCatalogOptions::Name(hstring const& value)
    {
        m_name = value;
    }

    Windows::Foundation::IReference<bool> EditPackageCatalogOptions::Explicit()
    {
        return m_explicit;
    }

    void EditPackageCatalogOptions::Explicit(Windows::Foundation::IReference<bool> value)
    {
        m_explicit = value;
    }

    Windows::Foundation::IReference<int32_t> EditPackageCatalogOptions::Priority()
    {
        return m_priority;
    }

    void EditPackageCatalogOptions::Priority(Windows::Foundation::IReference<int32_t> value)
    {
        m_priority = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(EditPackageCatalogOptions);
}
