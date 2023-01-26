// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageCatalogConnectOptions.h"
#pragma warning( pop )
#include "PackageCatalogConnectOptions.g.cpp"
#include "Helpers.h"


namespace winrt::Microsoft::Management::Deployment::implementation
{
    bool PackageCatalogConnectOptions::AcceptSourceAgreements()
    {
        return m_acceptSourceAgreements;
    }
    void PackageCatalogConnectOptions::AcceptSourceAgreements(bool value)
    {
        m_acceptSourceAgreements = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(PackageCatalogConnectOptions);
}
