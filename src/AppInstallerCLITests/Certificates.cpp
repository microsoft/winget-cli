// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Certificates.h>
#include <CertificateResources.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Certificates;


TEST_CASE("Certificates_NoPinningSucceeds", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::None);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_PublicKeyMismatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Rejected == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_PublicKeyMatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Root));
}

TEST_CASE("Certificates_SubjectMismatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Rejected == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Intermediate));
}

TEST_CASE("Certificates_SubjectMatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Intermediate));
}

TEST_CASE("Certificates_IssuerMismatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Issuer);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Rejected == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_IssuerMatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Issuer);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_ChainLengthDiffers", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!config.Validate(details.GetCertificate()));
}

TEST_CASE("Certificates_ChainLengthDiffers_Partial", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_AnyIssuer_Intermediate", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_AnyIssuer_IntermediateDiffers", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_1, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_AnyIssuer_IntermediateAndLeaf", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_AnyIssuer_Leaf", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_AnyIssuer_LeafDiffers", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_1, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_AnyIssuer_SameLeaf_RequireNonLeaf", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!config.Validate(details.GetCertificate()));
}

TEST_CASE("Certificates_EmptyChainRejects", "[certificates]")
{
    PinningChain chain;

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!config.Validate(details.GetCertificate()));
}

TEST_CASE("Certificates_ChainOrderDiffers", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!config.Validate(details.GetCertificate()));
}

TEST_CASE("Certificates_StoreChain_BuiltInTest", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));
}

TEST_CASE("Certificates_MultipleChains_Success", "[certificates]")
{
    PinningChain chainOutOfOrder;
    auto chainElement = chainOutOfOrder.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chainOutOfOrder);

    PinningChain chain;
    chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));
}

TEST_CASE("CertificateChain_Position", "[certificates]")
{
    CertificateChainPosition positions[3]{ CertificateChainPosition::Unknown, CertificateChainPosition::Unknown, CertificateChainPosition::Unknown };

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[0] = position; return true; });
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[1] = position; return true; });
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[2] = position; return true; });

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));

    REQUIRE(CertificateChainPosition::Root == positions[0]);
    REQUIRE(CertificateChainPosition::Intermediate == positions[1]);
    REQUIRE(CertificateChainPosition::Leaf == positions[2]);
}

TEST_CASE("CertificateChain_Position_SelfSigned", "[certificates]")
{
    CertificateChainPosition positions[1]{ CertificateChainPosition::Unknown };

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[0] = position; return true; });

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(config.Validate(details.GetCertificate()));

    REQUIRE((CertificateChainPosition::Root | CertificateChainPosition::Leaf) == positions[0]);
}
