// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wincrypt.h>

#include <json/json-forwards.h>
#include <wil/resource.h>

#include <functional>
#include <vector>


namespace AppInstaller::Certificates
{
    // Defines the types of certificate pinning to perform.
    enum class PinningVerificationType : uint32_t
    {
        None = 0x0,
        PublicKey = 0x1,
        Subject = 0x2,
        Issuer = 0x4,
    };

    DEFINE_ENUM_FLAG_OPERATORS(PinningVerificationType);

    // Contains the specific information about a certificate to pin.
    struct PinningDetails
    {
        PinningDetails() = default;

        PinningDetails(const PinningDetails&) = default;
        PinningDetails& operator=(const PinningDetails&) = default;

        PinningDetails(PinningDetails&&) = default;
        PinningDetails& operator=(PinningDetails&&) = default;

        // Loads the certificate context.
        PinningDetails& LoadCertificate(int resource, int resourceType);
        PinningDetails& LoadCertificate(const std::vector<BYTE>& certificateBytes);
        PinningDetails& LoadCertificate(const std::pair<const BYTE*,size_t> certificateBytes);
        PCCERT_CONTEXT GetCertificate() const { return m_certificateContext.get(); }

        PinningDetails& SetPinning(PinningVerificationType type);
        PinningVerificationType GetPinning() const { return m_pinning; }

        // Validates the given certificate against the pinning information.
        // Returns true to indicate that the certificate meets the pinning configuration criteria.
        // Returns false to indicate the it does not.
        bool Validate(PCCERT_CONTEXT certContext) const;

        // Loads the pinning details from the given JSON.
        [[nodiscard]] bool LoadFrom(const Json::Value& configuration);

    private:
        wil::shared_cert_context m_certificateContext;
        PinningVerificationType m_pinning = PinningVerificationType::None;
    };

    // Contains the full chain of pinning details.
    struct PinningChain
    {
        PinningChain() = default;

        PinningChain(const PinningChain&) = default;
        PinningChain& operator=(const PinningChain&) = default;

        PinningChain(PinningChain&&) = default;
        PinningChain& operator=(PinningChain&&) = default;

        // A single entry in the chain.
        struct Node
        {
            friend PinningChain;

            // Access the value
            PinningDetails* operator->() { return &m_chain.get()[m_index]; }

            // Create/access the next node in the chain.
            Node Next();
            const Node Next() const;

            // Drops the next node (and all subsequent nodes) from the chain.
            void RemoveNext();

            // Indicates if there is already an existing next node.
            bool HasNext() const;

        private:
            Node(std::vector<PinningDetails>& chain, size_t index);

            std::reference_wrapper<std::vector<PinningDetails>> m_chain;
            size_t m_index = 0;
        };

        // Gets the root certificate pinning details in the chain.
        // These will correspond to the root of the certificate chain being verified.
        Node Root();
        const Node Root() const;

        // Validates the given certificate chain against the configuration.
        // Returns true to indicate that the chain meets the pinning configuration criteria.
        // Returns false to indicate the it does not.
        bool Validate(PCCERT_CHAIN_CONTEXT chainContext) const;

        // Gets a description of the pinning chain.
        std::string GetDescription() const;

        // Loads the pinning chain from the given JSON.
        [[nodiscard]] bool LoadFrom(const Json::Value& configuration);

    private:
        std::vector<PinningDetails> m_chain;
    };

    // Holds the details about how a certificate chain is to be validated (aka "pinned").
    struct PinningConfiguration
    {
        PinningConfiguration(std::string identifier = {});

        PinningConfiguration(const PinningConfiguration&) = default;
        PinningConfiguration& operator=(const PinningConfiguration&) = default;

        PinningConfiguration(PinningConfiguration&&) = default;
        PinningConfiguration& operator=(PinningConfiguration&&) = default;

        // Adds a possible chain to the configuration.
        // For a certificate to be valid, it must match only one of the configured chains.
        void AddChain(PinningChain chain);

        // Validates the given leaf certificate against the configuration.
        // Returns true to indicate that the certificate meets the pinning configuration criteria.
        // Returns false to indicate the it does not.
        bool Validate(PCCERT_CONTEXT certContext) const;

        // True if no pinning is configured.
        bool IsEmpty() const { return m_configuration.empty(); }

        // Loads the pinning configuration from the given JSON.
        [[nodiscard]] bool LoadFrom(const Json::Value& configuration);

    private:
        // The identifier used when logging.
        std::string m_identifier;

        // The configured chains.
        std::vector<PinningChain> m_configuration;

        // We store the last certificate that was successfully validated to speed up subsequent checks.
        // Only cache a single certificate under the assumption that most of the time there will
        // only be a single server certificate in use.
        mutable std::vector<BYTE> m_cachedCertificate;
    };
}
