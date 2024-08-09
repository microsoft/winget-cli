// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageAgreement.g.h"
#include <winget/Manifest.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageAgreement : PackageAgreementT<PackageAgreement>
    {
        PackageAgreement() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Manifest::Agreement packageAgreement);
#endif

        hstring Label();
        hstring Text();
        hstring Url();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Manifest::Agreement m_packageAgreement{};
#endif
    };
}
