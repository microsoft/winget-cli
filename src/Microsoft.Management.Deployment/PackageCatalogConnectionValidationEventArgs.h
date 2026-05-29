// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogConnectionValidationEventArgs.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogConnectionValidationEventArgs : PackageCatalogConnectionValidationEventArgsT<PackageCatalogConnectionValidationEventArgs>
    {
        PackageCatalogConnectionValidationEventArgs() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(winrt::Windows::Security::Cryptography::Certificates::Certificate serverCertificate);
#endif

        winrt::Windows::Security::Cryptography::Certificates::Certificate ServerCertificate();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Windows::Security::Cryptography::Certificates::Certificate m_serverCertificate{ nullptr };
#endif
    };
}
