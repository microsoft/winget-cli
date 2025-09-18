// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Certificates.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"
#include "winget/JsonUtil.h"
#include "winget/Resources.h"

namespace AppInstaller::Certificates
{
    namespace
    {
        std::string GetNameString(PCCERT_CONTEXT certContext, DWORD nameType, bool forIssuer, void* typeParam = nullptr)
        {
            DWORD flags = forIssuer ? CERT_NAME_ISSUER_FLAG : 0;

            DWORD characterCount = CertGetNameStringW(certContext, nameType, flags, typeParam, nullptr, 0);
            std::wstring result(characterCount, L'\0');
            characterCount = CertGetNameStringW(certContext, nameType, flags, typeParam, &result[0], characterCount);

            if (static_cast<size_t>(characterCount) == result.size())
            {
                return Utility::ConvertToUTF8(static_cast<std::wstring_view>(result).substr(0, result.size() - 1));
            }
            else
            {
                return "<unknown>";
            }
        }

        std::string GetSimpleDisplayName(PCCERT_CONTEXT certContext, bool forIssuer = false)
        {
            return GetNameString(certContext, CERT_NAME_SIMPLE_DISPLAY_TYPE, forIssuer);
        }

        std::string GetX500Name(PCCERT_CONTEXT certContext, bool forIssuer = false)
        {
            DWORD stringType = CERT_X500_NAME_STR;
            return GetNameString(certContext, CERT_NAME_RDN_TYPE, forIssuer, &stringType);
        }

        std::string GetCommonName(PCCERT_CONTEXT certContext, bool forIssuer = false)
        {
            std::string commonName = szOID_COMMON_NAME;
            return GetNameString(certContext, CERT_NAME_ATTR_TYPE, forIssuer, &commonName[0]);
        }

        std::string GetDescriptionOfCertChain(PCCERT_CHAIN_CONTEXT chainContext)
        {
            PCCERT_SIMPLE_CHAIN chain = chainContext->rgpChain[0];
            std::ostringstream stream;
            std::string indent;

            for (DWORD i = 0; i < chain->cElement; ++i)
            {
                PCCERT_CHAIN_ELEMENT element = chain->rgpElement[(chain->cElement - 1) - i];

                if (!indent.empty())
                {
                    stream << std::endl;
                }

                stream << indent;

                stream << GetSimpleDisplayName(element->pCertContext);

                indent.append("  ");
            }

            return std::move(stream).str();
        }

        std::optional<PinningVerificationType> GetTypeFromString(std::string_view value)
        {
            std::string lowerValue = Utility::ToLower(value);

            if (lowerValue == "none")
            {
                return PinningVerificationType::None;
            }
            else if (lowerValue == "publickey")
            {
                return PinningVerificationType::PublicKey;
            }
            else if (lowerValue == "subject")
            {
                return PinningVerificationType::Subject;
            }
            else if (lowerValue == "issuer")
            {
                return PinningVerificationType::Issuer;
            }
            else if (lowerValue == "subjectcommonname")
            {
                return PinningVerificationType::SubjectCommonName;
            }
            else if (lowerValue == "issuercommonname")
            {
                return PinningVerificationType::IssuerCommonName;
            }

            return {};
        }

        bool CheckCommonName(const std::string& expectedName, const std::vector<std::string>& commonNames, const std::string& actualName, std::string_view logName)
        {
            bool foundAcceptedName = false;

            if (!expectedName.empty() && expectedName == actualName)
            {
                foundAcceptedName = true;
            }
            else
            {
                for (const std::string& name : commonNames)
                {
                    if (name == actualName)
                    {
                        foundAcceptedName = true;
                        break;
                    }
                }
            }

            if (!foundAcceptedName)
            {
                AICLI_LOG(Core, Verbose, << logName << " common name mismatch: Expected one of [" << expectedName << (expectedName.empty() || commonNames.empty() ? "" : ", ") << Utility::Join(", ", commonNames) << "], Actual common name [" << actualName << "]");
            }

            return foundAcceptedName;
        }

        void SetCommonNames(std::vector<std::string> names, std::vector<std::string>& target, bool append)
        {
            if (append)
            {
                for (auto& name : names)
                {
                    target.emplace_back(std::move(name));
                }
            }
            else
            {
                target = std::move(names);
            }
        }

        bool TryLoadCommonNames(const Json::Value& configuration, const std::string& memberName, std::vector<std::string>& target)
        {
            if (configuration.isMember(memberName))
            {
                auto memberValue = JSON::GetValue<std::vector<std::string>>(configuration[memberName]);
                if (!memberValue)
                {
                    AICLI_LOG(Core, Warning, << "Details JSON item member " << memberName << " was not an array of strings");
                    return false;
                }

                for (std::string& singleValue : memberValue.value())
                {
                    target.emplace_back(std::move(singleValue));
                }
            }

            return true;
        }
    }

    PinningDetails& PinningDetails::LoadCertificate(int resource, int resourceType)
    {
        return LoadCertificate(Resource::GetResourceAsBytes(resource, resourceType));
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

    void PinningDetails::SetSubjectCommonNames(std::vector<std::string> names, bool append)
    {
        SetCommonNames(names, m_subjectCommonNames, append);
    }

    void PinningDetails::SetIssuerCommonNames(std::vector<std::string> names, bool append)
    {
        SetCommonNames(names, m_issuerCommonNames, append);
    }

    PinningDetails& PinningDetails::SetPinning(PinningVerificationType type)
    {
        m_pinning = type;
        return *this;
    }

    // The JSON is expected to look like:
    // {
    //     "Validation":["publickey"],
    //     "EmbeddedCertificate":"<Hexadecimal string data for certificate>"
    // }
    bool PinningDetails::LoadFrom(const Json::Value& configuration)
    {
        const std::string validationName = "Validation";

        if (!configuration.isMember(validationName))
        {
            AICLI_LOG(Core, Warning, << "Details JSON item has no member " << validationName);
            return false;
        }

        auto validationValue = JSON::GetValue<std::vector<std::string>>(configuration[validationName]);
        if (!validationValue)
        {
            AICLI_LOG(Core, Warning, << "Details JSON item member " << validationName << " was not an array of strings");
            return false;
        }

        for (const std::string& singleValidation : validationValue.value())
        {
            auto validationType = GetTypeFromString(singleValidation);

            if (!validationType)
            {
                AICLI_LOG(Core, Warning, << "Details JSON validation is unknown: " << singleValidation);
                return false;
            }

            m_pinning |= validationType.value();
        }

        if (m_pinning == PinningVerificationType::None)
        {
            // No need to load a certificate if not doing any pinning
            return true;
        }

        const std::string embeddedCertificateName = "EmbeddedCertificate";

        if (configuration.isMember(embeddedCertificateName))
        {
            auto embeddedCertificateValue = JSON::GetValue<std::string>(configuration[embeddedCertificateName]);
            if (!validationValue)
            {
                AICLI_LOG(Core, Warning, << "Details JSON item member " << embeddedCertificateName << " was not a string");
                return false;
            }

            auto embeddedCertificateBytes = Utility::ParseFromHexString(embeddedCertificateValue.value());
            LoadCertificate(embeddedCertificateBytes);
        }

        if (!TryLoadCommonNames(configuration, "SubjectCommonNames", m_subjectCommonNames))
        {
            return false;
        }

        if (!TryLoadCommonNames(configuration, "IssuerCommonNames", m_issuerCommonNames))
        {
            return false;
        }

        return true;
    }

    bool PinningDetails::Validate(PCCERT_CONTEXT certContext) const
    {
        if (WI_IsFlagSet(m_pinning, PinningVerificationType::PublicKey))
        {
            THROW_HR_IF(E_NOT_VALID_STATE, !m_certificateContext);

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
            THROW_HR_IF(E_NOT_VALID_STATE, !m_certificateContext);

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
            THROW_HR_IF(E_NOT_VALID_STATE, !m_certificateContext);

            if (!CertCompareCertificateName(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                &m_certificateContext.get()->pCertInfo->Issuer,
                &certContext->pCertInfo->Issuer))
            {
                AICLI_LOG(Core, Verbose, << "Issuer mismatch: Expected certificate [" << GetSimpleDisplayName(m_certificateContext.get()) << "], Actual certificate [" << GetSimpleDisplayName(certContext) << "]");
                return false;
            }
        }

        if (WI_IsFlagSet(m_pinning, PinningVerificationType::SubjectCommonName))
        {
            std::string certificateSubject;
            if (m_certificateContext)
            {
                certificateSubject = GetCommonName(m_certificateContext.get());
            }

            if (!CheckCommonName(certificateSubject, m_subjectCommonNames, GetCommonName(certContext), "Subject"))
            {
                return false;
            }
        }

        if (WI_IsFlagSet(m_pinning, PinningVerificationType::IssuerCommonName))
        {
            std::string certificateIssuer;
            if (m_certificateContext)
            {
                certificateIssuer = GetCommonName(m_certificateContext.get(), true);
            }

            if (!CheckCommonName(certificateIssuer, m_issuerCommonNames, GetCommonName(certContext, true), "Issuer"))
            {
                return false;
            }
        }

        return true;
    }

    // TODO: Output common names and handle no certificate better
    void PinningDetails::OutputDescription(std::ostream& stream, std::string_view indent) const
    {
        if (m_certificateContext)
        {
            stream << GetSimpleDisplayName(m_certificateContext.get()) << " : ";
        }

        if (m_pinning == PinningVerificationType::None)
        {
            stream << "<No verification>";
        }

        bool prepend = false;

        for (const auto& flag : std::initializer_list<std::pair<PinningVerificationType, std::string_view>>{
            { PinningVerificationType::PublicKey, "PublicKey" },
            { PinningVerificationType::Subject, "Subject" },
            { PinningVerificationType::Issuer, "Issuer" },
            { PinningVerificationType::SubjectCommonName, "SubjectCommonName" },
            { PinningVerificationType::IssuerCommonName, "IssuerCommonName" },
            })
        {
            if (WI_IsAnyFlagSet(m_pinning, flag.first))
            {
                if (prepend)
                {
                    stream << " | ";
                }
                stream << flag.second;
                prepend = true;
            }
        }
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
            AICLI_LOG(Core, Warning, << "Empty pinning chain blindly rejecting chain context");
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

            details.OutputDescription(stream, indent);
            indent.append("  ");
        }

        return std::move(stream).str();
    }

    // The JSON is expected to look like:
    // {
    //     "Chain":[
    //         { <See PinningDetails::LoadFrom>
    //             "Validation":["publickey"],
    //             "EmbeddedCertificate":"<Hexadecimal string data for certificate>"
    //         },
    //         {
    //             "Validation":["subject","issuer"],
    //             "EmbeddedCertificate":"<Hexadecimal string data for certificate>"
    //         },
    //         ...
    //     ]
    // }
    bool PinningChain::LoadFrom(const Json::Value& configuration)
    {
        const std::string chainName = "Chain";
        if (!configuration.isMember(chainName))
        {
            AICLI_LOG(Core, Warning, << "Chains JSON item has no member " << chainName);
            return false;
        }

        const auto& chain = configuration[chainName];
        if (!chain.isArray())
        {
            AICLI_LOG(Core, Warning, << "Chain JSON input is not an array");
            return false;
        }

        for (const auto& configItem : chain)
        {
            PinningDetails details;
            if (!details.LoadFrom(configItem))
            {
                return false;
            }

            m_chain.emplace_back(std::move(details));
        }

        return true;
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
        AICLI_LOG(Core, Verbose, << "Adding chain to pinning configuration [" << m_identifier << "]:\n" << chain.GetDescription());
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
            AICLI_LOG(Core, Error, << "Rejecting certificate [" << GetSimpleDisplayName(certContext) << "] as it did not match anything in pinning configuration [" << m_identifier << "]:\n" << GetDescriptionOfCertChain(chainContext.get()));
        }

        return result;
    }

    // The JSON is expected to look like:
    // {
    //  "Chains":[
    //      { <See PinningChain::LoadFrom>
    //          "Chain":[
    //              { <See PinningDetails::LoadFrom>
    //                  "Validation":["publickey"],
    //                  "EmbeddedCertificate":"<Hexadecimal string data for certificate>"
    //              },
    //              {
    //                  "Validation":["subject","issuer"],
    //                  "EmbeddedCertificate":"<Hexadecimal string data for certificate>"
    //              },
    //              ...
    //          ]
    //      }
    //  ]
    // }
    bool PinningConfiguration::LoadFrom(const Json::Value& configuration)
    {
        const std::string chainsName = "Chains";
        if (!configuration.isMember(chainsName))
        {
            AICLI_LOG(Core, Warning, << "PinningConfiguration JSON item has no member " << chainsName);
            return false;
        }
        const auto& chains = configuration[chainsName];

        if (!chains.isArray())
        {
            AICLI_LOG(Core, Warning, << "PinningConfiguration.Chains is not an array");
            return false;
        }

        std::vector<PinningChain> resultCache;

        for (const auto& configItem : chains)
        {
            PinningChain chain;
            if (!chain.LoadFrom(configItem))
            {
                return false;
            }

            resultCache.emplace_back(std::move(chain));
        }

        // Move all chains into the config now that we have succeeded
        for (auto& result : resultCache)
        {
            AddChain(std::move(result));
        }

        return true;
    }
}
