// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wincrypt.h>

#include <json/forwards.h>
#include <wil/resource.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <vector>


namespace AppInstaller::Certificates
{
    // Returns the Authenticode signing subject name (e.g., "Microsoft Corporation") for the
    // given file. Only returns a value when WinVerifyTrust confirms the signature is valid and
    // trusted; extracts the subject from the embedded PE Authenticode signature via crypt32.
    // Returns an empty string if the file is unsigned, untrusted, or on any error.
    std::string GetAuthenticodeSubject(const std::filesystem::path& filePath);

    // Defines the types of certificate pinning to perform.
    enum class PinningVerificationType : uint32_t
    {
        // No validation; accepts anything.
        None = 0x0,
        // Validates that the public keys match; requires a certificate to be loaded.
        PublicKey = 0x1,
        // Validates that the full subjects match; requires a certificate to be loaded.
        Subject = 0x2,
        // Validates that the full issuers match; requires a certificate to be loaded.
        Issuer = 0x4,
        // Allows for unknown certificates in the chain; will continue to consume certificates from the chain until it matches.
        // Requires partial chain.
        AnyIssuer = 0x8,
        // Requires that the certificate is not a leaf.
        RequireNonLeaf = 0x10,
    };

    DEFINE_ENUM_FLAG_OPERATORS(PinningVerificationType);

    std::ostream& operator<<(std::ostream& out, PinningVerificationType value);

    // The position within a chain that a certificate is located.
    enum class CertificateChainPosition
    {
        Unknown = 0x0,
        // The start of the chain.
        Root = 0x1,
        // An indeterminate intermediate along the chain.
        Intermediate = 0x2,
        // The final certificate of the chain.
        Leaf = 0x4,
    };

    DEFINE_ENUM_FLAG_OPERATORS(CertificateChainPosition);

    std::ostream& operator<<(std::ostream& out, CertificateChainPosition value);

    // The result of validating a single certificate.
    enum class CertificatePinningValidationResult
    {
        // The certificate was accepted as valid.
        Accepted,
        // The certificate was rejected as invalid.
        Rejected,
        // The next certificate in the chain should be validated against the current details.
        // For use by partial chain validation that does not require exacting chain configurations.
        Skipped,
    };

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
        PinningDetails& LoadCertificate(const BYTE* certData, size_t certSize);
        PCCERT_CONTEXT GetCertificate() const { return m_certificateContext.get(); }

        PinningDetails& SetPinning(PinningVerificationType type);
        PinningVerificationType GetPinning() const { return m_pinning; }

        // Validates the given certificate against the pinning information.
        CertificatePinningValidationResult Validate(PCCERT_CONTEXT certContext, CertificateChainPosition position) const;

        // Outputs a description of the pinning details.
        void OutputDescription(std::ostream& stream, std::string_view indent) const;

        // Loads the pinning details from the given JSON.
        [[nodiscard]] bool LoadFrom(const Json::Value& configuration);

        // Determines how far the certificate is through its lifespan.
        double GetRemainingLifetimePercentage() const;

#ifndef AICLI_DISABLE_TEST_HOOKS
        using CustomValidationFunction = std::function<bool(const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition)>;
        void SetCustomValidationFunction(CustomValidationFunction function) { m_customValidation = std::move(function); }
    private:
        CustomValidationFunction m_customValidation;
#endif

    private:
        wil::shared_cert_context m_certificateContext;
        PinningVerificationType m_pinning = PinningVerificationType::None;
    };

    // Interface for a single chain validation strategy within a PinningConfiguration.
    // For a certificate to be accepted by PinningConfiguration, it must be accepted by at least one chain.
    struct IPinningChainValidation
    {
        virtual ~IPinningChainValidation() = default;

        // Returns true if the certificate chain is accepted.
        virtual bool Validate(PCCERT_CHAIN_CONTEXT chainContext) const = 0;

        // Returns a human-readable description of this validation for logging.
        virtual std::string GetDescription() const = 0;

        // Returns the remaining lifetime percentage of the pinned material (0.0 = expired, 1.0 = full life remaining).
        // Implementations without a fixed lifetime should return 1.0.
        virtual double GetRemainingLifetimePercentage() const = 0;
    };

    // Contains the full chain of pinning details.
    struct PinningChain : public IPinningChainValidation
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

        // A partial chain will validate success if all of its components are successful.
        PinningChain& PartialChain(bool isPartial = true);
        bool IsPartialChain() const { return m_partial; }

        // Validates the given certificate chain against the configuration.
        // Returns true to indicate that the chain meets the pinning configuration criteria.
        // Returns false to indicate the it does not.
        bool Validate(PCCERT_CHAIN_CONTEXT chainContext) const override;

        // Gets a description of the pinning chain.
        std::string GetDescription() const override;

        // Loads the pinning chain from the given JSON.
        [[nodiscard]] bool LoadFrom(const Json::Value& configuration);

        // Determines how far the certificate chain is through its lifespan (the minimum of all of its certificates).
        double GetRemainingLifetimePercentage() const override;

    private:
        std::vector<PinningDetails> m_chain;
        bool m_partial = false;
    };

    // A chain validation that delegates to a caller-supplied callback.
    struct CallbackPinningChainValidation : public IPinningChainValidation
    {
        // callback receives the leaf (end-entity) certificate and returns true to accept, false to reject.
        explicit CallbackPinningChainValidation(std::function<bool(PCCERT_CONTEXT)> callback);

        bool Validate(PCCERT_CHAIN_CONTEXT chainContext) const override;
        std::string GetDescription() const override;
        double GetRemainingLifetimePercentage() const override { return 1.0; }

    private:
        std::function<bool(PCCERT_CONTEXT)> m_callback;
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
        void AddChain(std::shared_ptr<IPinningChainValidation> chain);

        // Builds a certificate chain for the given leaf certificate.
        // Provide a custom chain engine and additional store to use non-system-trusted roots
        // (e.g., for testing with self-signed certificates).
        // Pass nullptr for engine to use the system default engine.
        // Pass nullptr for additionalStore to use only certContext->hCertStore.
        // Pass 0 for flags to skip revocation checking (e.g., for test certs with no CRL).
        static wil::unique_cert_chain_context BuildCertificateChain(
            PCCERT_CONTEXT certContext,
            HCERTCHAINENGINE engine = nullptr,
            HCERTSTORE additionalStore = nullptr,
            DWORD flags = CERT_CHAIN_REVOCATION_CHECK_CHAIN);

        // Validates the given leaf certificate against the configuration using a pre-built chain.
        // Use BuildCertificateChain to construct the chain, providing a custom engine if needed.
        bool Validate(PCCERT_CONTEXT certContext, PCCERT_CHAIN_CONTEXT chainContext) const;

        // Validates the given leaf certificate against the configuration.
        // Builds the certificate chain internally using the system chain engine.
        bool Validate(PCCERT_CONTEXT certContext) const;

        // True if no pinning is configured.
        bool IsEmpty() const { return m_configuration.empty(); }

        // Loads the pinning configuration from the given JSON.
        [[nodiscard]] bool LoadFrom(const Json::Value& configuration);

        // Determines how far the configuration is through its lifespan (the maximum of all of its chains).
        double GetRemainingLifetimePercentage() const;

    private:
        // The identifier used when logging.
        std::string m_identifier;

        // The configured chains.
        std::vector<std::shared_ptr<IPinningChainValidation>> m_configuration;

        // We store the last certificate that was successfully validated to speed up subsequent checks.
        // Only cache a single certificate under the assumption that most of the time there will
        // only be a single server certificate in use.
        mutable std::vector<BYTE> m_cachedCertificate;
    };
}
