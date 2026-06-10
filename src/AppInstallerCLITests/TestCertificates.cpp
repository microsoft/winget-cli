// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCertificates.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>

namespace TestCommon
{
    namespace
    {
        AppInstaller::Certificates::PinningDetails LoadCertFromTestData(const char* filename)
        {
            std::filesystem::path path = TestDataFile(filename).GetPath();
            std::ifstream file(path, std::ios::binary);
            THROW_HR_IF(E_FAIL, !file.is_open());
            auto bytes = AppInstaller::Utility::ReadEntireStreamAsByteArray(file);
            AppInstaller::Certificates::PinningDetails details;
            details.LoadCertificate(bytes);
            return details;
        }

        wil::unique_hcertstore OpenMemoryStore()
        {
            wil::unique_hcertstore store{ CertOpenStore(CERT_STORE_PROV_MEMORY, 0, 0, 0, nullptr) };
            THROW_LAST_ERROR_IF(!store);
            return store;
        }
    }

    TestCertificateChain::TestCertificateChain()
    {
        m_root          = LoadCertFromTestData("TestRoot.cer");
        m_intermediate1 = LoadCertFromTestData("TestIntermediate1.cer");
        m_intermediate2 = LoadCertFromTestData("TestIntermediate2.cer");
        m_leaf1         = LoadCertFromTestData("TestLeaf1.cer");
        m_leaf2         = LoadCertFromTestData("TestLeaf2.cer");

        // Root store: used as hExclusiveRoot so only TestRoot is trusted.
        m_rootStore = OpenMemoryStore();
        THROW_IF_WIN32_BOOL_FALSE(CertAddCertificateContextToStore(m_rootStore.get(), m_root.GetCertificate(), CERT_STORE_ADD_NEW, nullptr));

        CERT_CHAIN_ENGINE_CONFIG engineConfig = {};
        engineConfig.cbSize = sizeof(engineConfig);
        engineConfig.hExclusiveRoot = m_rootStore.get();
        HCERTCHAINENGINE rawEngine = nullptr;
        THROW_IF_WIN32_BOOL_FALSE(CertCreateCertificateChainEngine(&engineConfig, &rawEngine));
        m_chainEngine.reset(rawEngine);

        // Additional store: holds intermediates so the chain builder can find issuers.
        m_additionalStore = OpenMemoryStore();
        THROW_IF_WIN32_BOOL_FALSE(CertAddCertificateContextToStore(m_additionalStore.get(), m_intermediate1.GetCertificate(), CERT_STORE_ADD_NEW, nullptr));
        THROW_IF_WIN32_BOOL_FALSE(CertAddCertificateContextToStore(m_additionalStore.get(), m_intermediate2.GetCertificate(), CERT_STORE_ADD_NEW, nullptr));
    }

    wil::unique_cert_chain_context TestCertificateChain::BuildChain(PCCERT_CONTEXT certContext) const
    {
        return AppInstaller::Certificates::PinningConfiguration::BuildCertificateChain(
            certContext,
            m_chainEngine.get(),
            m_additionalStore.get(),
            0); // No revocation checking for test certs (no CRL endpoints)
    }

    CertContextRef TestCertificateChain::Root() const         { return m_root.GetCertificate(); }
    CertContextRef TestCertificateChain::Intermediate1() const { return m_intermediate1.GetCertificate(); }
    CertContextRef TestCertificateChain::Intermediate2() const { return m_intermediate2.GetCertificate(); }
    CertContextRef TestCertificateChain::Leaf1() const         { return m_leaf1.GetCertificate(); }
    CertContextRef TestCertificateChain::Leaf2() const         { return m_leaf2.GetCertificate(); }
}
