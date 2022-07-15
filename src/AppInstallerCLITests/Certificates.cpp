// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Certificates.h>
#include <CertificateResources.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Certificates;


TEST_CASE("Certificates_StoreChain_BuiltInTest", "[certificates]")
{
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_1).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_1).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_1).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    PinningDetails details;
    details.LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_1);

    REQUIRE(config.Validate(details.GetCertificate()));
}
