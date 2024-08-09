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

    REQUIRE(expected.Validate(actual.GetCertificate()));
}

TEST_CASE("Certificates_PublicKeyMismatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!expected.Validate(actual.GetCertificate()));
}

TEST_CASE("Certificates_PublicKeyMatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::PublicKey);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(expected.Validate(actual.GetCertificate()));
}

TEST_CASE("Certificates_SubjectMismatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!expected.Validate(actual.GetCertificate()));
}

TEST_CASE("Certificates_SubjectMatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Subject);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(expected.Validate(actual.GetCertificate()));
}

TEST_CASE("Certificates_IssuerMismatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Issuer);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(!expected.Validate(actual.GetCertificate()));
}

TEST_CASE("Certificates_IssuerMatch", "[certificates]")
{
    PinningDetails expected;
    expected.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE).SetPinning(PinningVerificationType::Issuer);

    PinningDetails actual;
    actual.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_2, CERTIFICATE_RESOURCE_TYPE);

    REQUIRE(expected.Validate(actual.GetCertificate()));
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
