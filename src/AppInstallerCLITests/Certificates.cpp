// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestCertificates.h"
#include <winget/Certificates.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Certificates;


TEST_CASE("Certificates_NoPinningSucceeds", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::None);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Leaf2().View());

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_PublicKeyMismatch", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Leaf2().View());

    REQUIRE(CertificatePinningValidationResult::Rejected == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_PublicKeyMatch", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Root().View());

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Root));
}

TEST_CASE("Certificates_SubjectMismatch", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::Subject);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Intermediate2().View());

    REQUIRE(CertificatePinningValidationResult::Rejected == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Intermediate));
}

TEST_CASE("Certificates_SubjectMatch", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Subject);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Intermediate2().View());

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Intermediate));
}

TEST_CASE("Certificates_IssuerMismatch", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Issuer);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Leaf2().View());

    REQUIRE(CertificatePinningValidationResult::Rejected == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_IssuerMatch", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningDetails expected;
    expected.LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Issuer);

    PinningDetails actual;
    actual.LoadCertificate(testChain.Leaf2().View());

    REQUIRE(CertificatePinningValidationResult::Accepted == expected.Validate(actual.GetCertificate(), CertificateChainPosition::Leaf));
}

TEST_CASE("Certificates_ChainLengthDiffers", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(!config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("Certificates_ChainLengthDiffers_Partial", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_AnyIssuer_Intermediate", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_AnyIssuer_IntermediateDiffers", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Intermediate1().View()).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(!config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_AnyIssuer_IntermediateAndLeaf", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_AnyIssuer_Leaf", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_AnyIssuer_LeafDiffers", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Leaf1().View()).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(!config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_AnyIssuer_SameLeaf_RequireNonLeaf", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::PublicKey | PinningVerificationType::AnyIssuer | PinningVerificationType::RequireNonLeaf);
    chain.PartialChain();

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(!config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("Certificates_EmptyChainRejects", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(!config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("Certificates_ChainOrderDiffers", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(!config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("Certificates_StoreChain_BuiltInTest", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("Certificates_MultipleChains_Success", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    PinningChain chainOutOfOrder;
    auto chainElement = chainOutOfOrder.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chainOutOfOrder);

    PinningChain chain;
    chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));
}

TEST_CASE("CertificateChain_Position", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    CertificateChainPosition positions[3]{ CertificateChainPosition::Unknown, CertificateChainPosition::Unknown, CertificateChainPosition::Unknown };

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[0] = position; return true; });
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Intermediate2().View()).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[1] = position; return true; });
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(testChain.Leaf2().View()).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[2] = position; return true; });

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Leaf2());
    REQUIRE(config.Validate(testChain.Leaf2(), chainCtx.get()));

    REQUIRE(CertificateChainPosition::Root == positions[0]);
    REQUIRE(CertificateChainPosition::Intermediate == positions[1]);
    REQUIRE(CertificateChainPosition::Leaf == positions[2]);
}

TEST_CASE("CertificateChain_Position_SelfSigned", "[certificates][uses-test-certificates]")
{
    TestCertificateChain testChain;

    CertificateChainPosition positions[1]{ CertificateChainPosition::Unknown };

    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(testChain.Root().View()).SetCustomValidationFunction([&](const PinningDetails&, PCCERT_CONTEXT, CertificateChainPosition position) { positions[0] = position; return true; });

    PinningConfiguration config;
    config.AddChain(chain);

    auto chainCtx = testChain.BuildChain(testChain.Root());
    REQUIRE(config.Validate(testChain.Root(), chainCtx.get()));

    REQUIRE((CertificateChainPosition::Root | CertificateChainPosition::Leaf) == positions[0]);
}
