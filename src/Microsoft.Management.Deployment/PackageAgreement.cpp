// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageAgreement.h"
#include "PackageAgreement.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageAgreement::Initialize(::AppInstaller::Manifest::Agreement packageAgreement)
    {
        m_packageAgreement = std::move(packageAgreement);
    }
    hstring PackageAgreement::Label()
    {
        return winrt::to_hstring(m_packageAgreement.Label);
    }
    hstring PackageAgreement::Text()
    {
        return winrt::to_hstring(m_packageAgreement.AgreementText);
    }
    hstring PackageAgreement::Url()
    {
        return winrt::to_hstring(m_packageAgreement.AgreementUrl);
    }
}
