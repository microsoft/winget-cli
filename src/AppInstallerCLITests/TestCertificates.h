// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestCommon.h"
#include <winget/Certificates.h>
#include <wil/resource.h>

namespace TestCommon
{
    using unique_cert_chain_engine = wil::unique_any<HCERTCHAINENGINE, decltype(&::CertFreeCertificateChainEngine), ::CertFreeCertificateChainEngine>;

    // Non-owning view of a PCCERT_CONTEXT.
    // operator-> accesses the underlying CERT_CONTEXT fields directly.
    // View() returns the encoded bytes as a pair suitable for PinningDetails::LoadCertificate.
    // Implicitly converts to PCCERT_CONTEXT for APIs that take the raw handle.
    struct CertContextRef
    {
        CertContextRef(PCCERT_CONTEXT cert) : m_cert(cert) {}

        PCCERT_CONTEXT operator->() const { return m_cert; }
        operator PCCERT_CONTEXT() const { return m_cert; }

        std::pair<const BYTE*, size_t> View() const
        {
            return { m_cert->pbCertEncoded, static_cast<size_t>(m_cert->cbCertEncoded) };
        }

    private:
        PCCERT_CONTEXT m_cert;
    };

    // Provides a self-contained PKI hierarchy for unit tests, loaded from TestData files.
    // The hierarchy is:
    //   TestRoot  ->  TestIntermediate1  ->  TestLeaf1
    //             ->  TestIntermediate2  ->  TestLeaf2
    //
    // Use BuildChain() to get a PCCERT_CHAIN_CONTEXT built with a custom chain engine that
    // trusts TestRoot exclusively, suitable for passing to PinningConfiguration::Validate().
    // Revocation checking is disabled since the test certs have no CRL.
    struct TestCertificateChain
    {
        TestCertificateChain();

        TestCertificateChain(const TestCertificateChain&) = delete;
        TestCertificateChain& operator=(const TestCertificateChain&) = delete;

        // Builds a certificate chain for certContext using the test chain engine.
        wil::unique_cert_chain_context BuildChain(PCCERT_CONTEXT certContext) const;

        CertContextRef Root() const;
        CertContextRef Intermediate1() const;
        CertContextRef Intermediate2() const;
        CertContextRef Leaf1() const;
        CertContextRef Leaf2() const;

    private:
        AppInstaller::Certificates::PinningDetails m_root;
        AppInstaller::Certificates::PinningDetails m_intermediate1;
        AppInstaller::Certificates::PinningDetails m_intermediate2;
        AppInstaller::Certificates::PinningDetails m_leaf1;
        AppInstaller::Certificates::PinningDetails m_leaf2;

        unique_cert_chain_engine m_chainEngine;
        wil::unique_hcertstore m_rootStore;
        wil::unique_hcertstore m_additionalStore;
    };
}
