// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Certificates.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"
#include "winget/Resources.h"

#include <CertificateResources.h>


namespace AppInstaller::Certificates
{
    namespace
    {
        std::string GetSimpleDisplayName(PCCERT_CONTEXT certContext)
        {
            DWORD characterCount = CertGetNameStringW(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, nullptr, 0);
            std::wstring result(characterCount, L'\0');
            characterCount = CertGetNameStringW(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr, &result[0], characterCount);

            if (static_cast<size_t>(characterCount) == result.size())
            {
                return Utility::ConvertToUTF8(static_cast<std::wstring_view>(result).substr(0, result.size() - 1));
            }
            else
            {
                return "<unknown>";
            }
        }
    }

    PinningDetails& PinningDetails::LoadCertificate(int resource)
    {
        return LoadCertificate(Resource::GetResourceAsBytes(resource, CERTIFICATE_RESOURCE_TYPE));
    }

    PinningDetails& PinningDetails::LoadCertificate(const std::vector<BYTE>& certificateBytes)
    {
        return LoadCertificate(std::make_pair(&certificateBytes[0], certificateBytes.size()));
    }

    PinningDetails& PinningDetails::LoadCertificate(const std::pair<const BYTE*, size_t> certificateBytes)
    {
        m_certificateContext.reset(CertCreateCertificateContext(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, certificateBytes.first, static_cast<DWORD>(certificateBytes.second)));
        THROW_LAST_ERROR_IF(!m_certificateContext);
        return *this;
    }

    PinningDetails& PinningDetails::SetPinning(PinningVerificationType type)
    {
        m_pinning = type;
        return *this;
    }

    bool PinningDetails::Validate(PCCERT_CONTEXT certContext) const
    {
        if (WI_IsFlagSet(m_pinning, PinningVerificationType::PublicKey))
        {
            if (!CertComparePublicKeyInfo(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &m_certificateContext.get()->pCertInfo->SubjectPublicKeyInfo,
                &certContext->pCertInfo->SubjectPublicKeyInfo))
            {
                AICLI_LOG(Core, Verbose, << "Public key mismatch: Expected certificate [" << GetSimpleDisplayName(m_certificateContext.get()) << "], Actual certificate [" << GetSimpleDisplayName(certContext) << "]");
                return false;
            }
        }

        if (WI_IsFlagSet(m_pinning, PinningVerificationType::Subject))
        {
            if (!CertCompareCertificateName(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &m_certificateContext.get()->pCertInfo->Subject,
                &certContext->pCertInfo->Subject))
            {
                AICLI_LOG(Core, Verbose, << "Subject mismatch: Expected certificate [" << GetSimpleDisplayName(m_certificateContext.get()) << "], Actual certificate [" << GetSimpleDisplayName(certContext) << "]");
                return false;
            }
        }

        if (WI_IsFlagSet(m_pinning, PinningVerificationType::Issuer))
        {
            if (!CertCompareCertificateName(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &m_certificateContext.get()->pCertInfo->Issuer,
                &certContext->pCertInfo->Issuer))
            {
                AICLI_LOG(Core, Verbose, << "Issuer mismatch: Expected certificate [" << GetSimpleDisplayName(m_certificateContext.get()) << "], Actual certificate [" << GetSimpleDisplayName(certContext) << "]");
                return false;
            }
        }

        return true;
    }

    PinningChain::Node PinningChain::Node::Next()
    {
        if (!HasNext())
        {
            m_chain.get().emplace_back();
        }

        return { m_chain, m_index + 1 };
    }

    const PinningChain::Node PinningChain::Node::Next() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !HasNext());
        return { m_chain, m_index + 1 };
    }

    void PinningChain::Node::RemoveNext()
    {
        m_chain.get().erase(m_chain.get().begin() + m_index + 1, m_chain.get().end());
    }

    bool PinningChain::Node::HasNext() const
    {
        return (m_index + 1 < m_chain.get().size());
    }

    PinningChain::Node::Node(std::vector<PinningDetails>& chain, size_t index) :
        m_chain(chain), m_index(index) {}

    PinningChain::Node PinningChain::Root()
    {
        if (m_chain.empty())
        {
            m_chain.emplace_back();
        }

        return { m_chain, 0 };
    }

    const PinningChain::Node PinningChain::Root() const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_chain.empty());
        return { const_cast<std::vector<PinningDetails>&>(m_chain), 0 };
    }

    bool PinningChain::Validate(PCCERT_CHAIN_CONTEXT chainContext) const
    {
        if (m_chain.empty())
        {
            // An empty chain rejects all inputs.
            AICLI_LOG(Core, Verbose, << "Empty pinning chain blindly rejecting chain context");
            return false;
        }

        THROW_HR_IF(E_INVALIDARG, chainContext->cChain == 0);

        // Currently don't support chains bridged with CTLs; there must be only one simple chain that terminates in a trusted root.
        if (chainContext->cChain > 1)
        {
            AICLI_LOG(Core, Verbose, << "Rejecting chain context with multiple chains");
            return false;
        }

        PCCERT_SIMPLE_CHAIN chain = chainContext->rgpChain[0];

        if (chain->TrustStatus.dwErrorStatus != CERT_TRUST_NO_ERROR)
        {
            AICLI_LOG(Core, Verbose, << "Rejecting simple chain context with bad TrustStatus: " << chain->TrustStatus.dwErrorStatus << " [" << chain->TrustStatus.dwInfoStatus << "]");
            return false;
        }

        if (chain->pTrustListInfo)
        {
            // This should not happen as the only reason for pTrustListInfo to be set is when `chainContext->cChain > 1`, which is rejected above
            AICLI_LOG(Core, Verbose, << "Rejecting simple chain context with CTL info");
            return false;
        }

        if (static_cast<size_t>(chain->cElement) != m_chain.size())
        {
            AICLI_LOG(Core, Verbose, << "Rejecting simple chain context based on size: expected " << m_chain.size() << ", got " << chain->cElement);
            return false;
        }

        for (DWORD i = 0; i < chain->cElement; ++i)
        {
            PCCERT_CHAIN_ELEMENT element = chain->rgpElement[(chain->cElement - 1) - i];

            if (element->TrustStatus.dwErrorStatus != CERT_TRUST_NO_ERROR)
            {
                AICLI_LOG(Core, Verbose, << "Rejecting chain element with bad TrustStatus: " << element->TrustStatus.dwErrorStatus << " [" << element->TrustStatus.dwInfoStatus << "]");
                return false;
            }

            if (!m_chain[i].Validate(element->pCertContext))
            {
                return false;
            }
        }

        return true;
    }

    std::string PinningChain::GetDescription() const
    {
        if (m_chain.empty())
        {
            return "<empty>";
        }

        std::ostringstream stream;
        std::string indent;

        for (const PinningDetails& details : m_chain)
        {
            if (!indent.empty())
            {
                stream << std::endl;
            }

            stream << indent;

            if (details.GetPinning() == PinningVerificationType::None)
            {
                stream << "<No verification>";
            }
            else
            {
                stream << indent << GetSimpleDisplayName(details.GetCertificate()) << " : ";

                bool prepend = false;

                if (WI_IsFlagSet(details.GetPinning(), PinningVerificationType::PublicKey))
                {
                    stream << "PublicKey";
                    prepend = true;
                }

                if (WI_IsFlagSet(details.GetPinning(), PinningVerificationType::Subject))
                {
                    if (prepend)
                    {
                        stream << " | ";
                    }
                    stream << "Subject";
                    prepend = true;
                }

                if (WI_IsFlagSet(details.GetPinning(), PinningVerificationType::Issuer))
                {
                    if (prepend)
                    {
                        stream << " | ";
                    }
                    stream << "Issuer";
                    prepend = true;
                }
            }

            indent.append("  ");
        }

        return std::move(stream).str();
    }

    PinningConfiguration::PinningConfiguration(std::string identifier) : m_identifier(identifier)
    {
        if (m_identifier.empty())
        {
            GUID guid;
            LOG_IF_FAILED(CoCreateGuid(&guid));
            wchar_t identifierBuffer[256] = {};
            (void)StringFromGUID2(guid, identifierBuffer, ARRAYSIZE(identifierBuffer));
            m_identifier = Utility::ConvertToUTF8(identifierBuffer);
        }
    }

    void PinningConfiguration::AddChain(PinningChain chain)
    {
        AICLI_LOG(Core, Info, << "Adding chain to pinning configuration [" << m_identifier << "]:\n" << chain.GetDescription());
        m_configuration.emplace_back(std::move(chain));
    }

    bool PinningConfiguration::Validate(PCCERT_CONTEXT certContext) const
    {
        if (m_configuration.empty())
        {
            // No pinning configured
            return true;
        }

        const BYTE* encodedBegin = certContext->pbCertEncoded;
        const BYTE* encodedEnd = encodedBegin + certContext->cbCertEncoded;
        if (certContext->cbCertEncoded == m_cachedCertificate.size() &&
            std::equal(encodedBegin, encodedEnd, m_cachedCertificate.begin()))
        {
            // We have seen this certificate and deemed it valid already.
            return true;
        }

        // Get the chain for the given leaf certificate
        wil::unique_cert_chain_context chainContext;

        char oidPkixKpServerAuth[] = szOID_PKIX_KP_SERVER_AUTH;
        std::array<char*, 1> chainUses = {
            oidPkixKpServerAuth,
        };

        CERT_CHAIN_PARA chainParameters = {};
        chainParameters.cbSize = sizeof(chainParameters);
        chainParameters.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
        chainParameters.RequestedUsage.Usage.cUsageIdentifier = static_cast<DWORD>(chainUses.size());
        chainParameters.RequestedUsage.Usage.rgpszUsageIdentifier = chainUses.data();

        THROW_IF_WIN32_BOOL_FALSE(CertGetCertificateChain(nullptr, certContext, nullptr, certContext->hCertStore, &chainParameters, CERT_CHAIN_REVOCATION_CHECK_CHAIN, nullptr, &chainContext));

        bool result = false;

        for (const auto& chain : m_configuration)
        {
            if (chain.Validate(chainContext.get()))
            {
                result = true;
                break;
            }
        }

        if (result)
        {
            // Only cache a successful validation
            m_cachedCertificate.assign(encodedBegin, encodedEnd);
        }
        else
        {
            AICLI_LOG(Core, Error, << "Rejecting certificate [" << GetSimpleDisplayName(certContext) << "] as it did not match anything in pinning configuration [" << m_identifier << "]");
        }

        return result;
    }
}
