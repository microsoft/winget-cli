// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/RepositorySource.h>
#include "SourceAgreement.h"
#include "SourceAgreement.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void SourceAgreement::Initialize(::AppInstaller::Repository::SourceAgreement sourceAgreement)
    {
        m_sourceAgreement = std::move(sourceAgreement);
    }
    hstring SourceAgreement::Label()
    {
        return winrt::to_hstring(m_sourceAgreement.Label);
    }
    hstring SourceAgreement::Text()
    {
        return winrt::to_hstring(m_sourceAgreement.Text);
    }
    hstring SourceAgreement::Url()
    {
        return winrt::to_hstring(m_sourceAgreement.Url);
    }
}
