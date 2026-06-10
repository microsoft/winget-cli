// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageCatalogConnectionValidationEventArgs.h"
#include "PackageCatalogConnectionValidationEventArgs.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void PackageCatalogConnectionValidationEventArgs::Initialize(winrt::Windows::Security::Cryptography::Certificates::Certificate serverCertificate)
    {
        m_serverCertificate = serverCertificate;
    }

    winrt::Windows::Security::Cryptography::Certificates::Certificate PackageCatalogConnectionValidationEventArgs::ServerCertificate()
    {
        return m_serverCertificate;
    }
}
