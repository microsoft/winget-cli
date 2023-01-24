// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "ConnectOptions.h"
#pragma warning( pop )
#include "ConnectOptions.g.cpp"
#include "Helpers.h"


namespace winrt::Microsoft::Management::Deployment::implementation
{
    ConnectOptions::ConnectOptions()
    {
    }
    winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::SourceAgreement> ConnectOptions::SourceAgreements()
    {
        return m_sourceAgreements;
    }
    void ConnectOptions::SourceAgreements(winrt::Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::SourceAgreement> const& value)
    {
        m_sourceAgreements = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(ConnectOptions);
}
