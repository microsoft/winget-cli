// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SourceAgreement.g.h"
#include <winget/RepositorySource.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct SourceAgreement : SourceAgreementT<SourceAgreement>
    {
        SourceAgreement() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Repository::SourceAgreement sourceAgreement);
#endif

        hstring Label();
        hstring Text();
        hstring Url();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Repository::SourceAgreement m_sourceAgreement{};
#endif
    };
}
