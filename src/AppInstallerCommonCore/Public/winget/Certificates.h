// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wincrypt.h>


namespace AppInstaller::Certificates
{
#define WINGET_CERTIFICATES_DETAILS_PROPERTY(_name_,_type_) \
    public: \
        CertificateDetails& _name_(std::optional<_type_> value) { m_ ## _name_ = std::move(value); } \
        const std::optional<_type_>& _name_() const { return m_ ## _name_; } \
    private: \
        std::optional<_type_> m_ ## _name_

    // Contains the specific information about a certificate to check.
    struct CertificateDetails
    {
        // For public key pinning; provide all 4 pieces of information to ensure an exact match.
        // The OID of the algorithm, as in CERT_PUBLIC_KEY_INFO
        WINGET_CERTIFICATES_DETAILS_PROPERTY(PublicKeyAlgorithm, std::string);
        // The parameters of the algorithm, as in CERT_PUBLIC_KEY_INFO
        WINGET_CERTIFICATES_DETAILS_PROPERTY(PublicKeyAlgorithmParameters, std::vector<BYTE>);
        // The number of bits in the public key, as in CERT_PUBLIC_KEY_INFO
        WINGET_CERTIFICATES_DETAILS_PROPERTY(PublicKeyBitCount, size_t);
        // The bytes of the public key, as in CERT_PUBLIC_KEY_INFO
        WINGET_CERTIFICATES_DETAILS_PROPERTY(PublicKeyBytes, size_t);

        // The full subject of the certificate, as in CERT_INFO
        WINGET_CERTIFICATES_DETAILS_PROPERTY(Subject, std::vector<BYTE>);
        // The full issuer of the certificate, as in CERT_INFO
        WINGET_CERTIFICATES_DETAILS_PROPERTY(Issuer, std::vector<BYTE>);
    };

    // Contains the certificate details and information about how the chain is expected to look.
    struct ChainDetails
    {


    private:
        std::vector<CertificateDetails> m_chainDetails;
    };

    // Holds the details about how a certificate chain is to be validated (aka "pinned").
    struct PinningConfiguration
    {
        PinningConfiguration() = default;

        // Validates the given certificate against the configuration.
        // Returns true to indicate that the certificate meets the pinning configuration criteria.
        // Returns false to indicate the it does not.
        bool Validate(PCCERT_CONTEXT certContext) const;

    private:

    };
}
