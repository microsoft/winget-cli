// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "RemovePackageCatalogOptions.h"
#pragma warning( pop )
#include "RemovePackageCatalogOptions.g.cpp"
#include "Converters.h"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring RemovePackageCatalogOptions::Name()
    {
        return hstring(m_name);
    }
    void RemovePackageCatalogOptions::Name(hstring const& value)
    {
        m_name = value;
    }
    bool RemovePackageCatalogOptions::PreserveData()
    {
        return m_preserveData;
    }
    void RemovePackageCatalogOptions::PreserveData(bool const& value)
    {
        m_preserveData = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(RemovePackageCatalogOptions);
}
