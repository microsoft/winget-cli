// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/RepositorySource.h>
#include "Workflows/WorkflowBase.h"
#include "Converters.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "PackageMatchFilter.h"
#pragma warning( pop )
#include "PackageMatchFilter.g.cpp"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageMatchFilter::Initialize(::AppInstaller::Repository::PackageMatchFilter matchFilter)
    {
        m_value = winrt::to_hstring(matchFilter.Value);
        m_matchField = GetDeploymentMatchField(matchFilter.Field);
        m_packageFieldMatchOption = GetDeploymentMatchOption(matchFilter.Type);
    }
    winrt::Microsoft::Management::Deployment::PackageFieldMatchOption PackageMatchFilter::Option()
    {
        return m_packageFieldMatchOption;
    }
    void PackageMatchFilter::Option(winrt::Microsoft::Management::Deployment::PackageFieldMatchOption const& value)
    {
        m_packageFieldMatchOption = value;
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

    CoCreatableMicrosoftManagementDeploymentClass(PackageMatchFilter);
}
