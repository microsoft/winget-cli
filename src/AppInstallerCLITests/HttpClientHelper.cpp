// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <AppInstallerErrors.h>
#include <Rest/Schema/HttpClientHelper.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/Certificates.h>
#include <CertificateResources.h>

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Runtime;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Certificates;

TEST_CASE("ExtractJsonResponse_UnsupportedMimeType", "[RestSource][RestSearch]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::OK, L"", web::http::details::mime_types::text_plain) };
    REQUIRE_THROWS_HR(helper.HandleGet(L"https://testUri"), APPINSTALLER_CLI_ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE);
}

TEST_CASE("ValidateAndExtractResponse_ServiceUnavailable", "[RestSource]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::ServiceUnavailable) };
    REQUIRE_THROWS_HR(helper.HandleGet(L"https://testUri"), MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, web::http::status_codes::ServiceUnavailable));
}

TEST_CASE("ValidateAndExtractResponse_NotFound", "[RestSource]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler(web::http::status_codes::NotFound) };
    REQUIRE_THROWS_HR(helper.HandleGet(L"https://testUri"), APPINSTALLER_CLI_ERROR_RESTSOURCE_ENDPOINT_NOT_FOUND);
}

TEST_CASE("EnsureDefaultUserAgent", "[RestSource]")
{
    HttpClientHelper helper{ GetTestRestRequestHandler([](const web::http::http_request& request)
        {
            auto itr = request.headers().find(web::http::header_names::user_agent);
            if (itr != request.headers().end() &&
                itr->second.find(ConvertToUTF16(GetClientVersion())) != utility::string_t::npos &&
                itr->second.find(ConvertToUTF16(GetPackageVersion())) != utility::string_t::npos)
            {
                return web::http::status_codes::OK;
            }
            else
            {
                return web::http::status_codes::BadRequest;
            }
        }) };

    SECTION("GET")
    {
        REQUIRE_NOTHROW(helper.HandleGet(L"https://testUri"));
    }
    SECTION("POST")
    {
        REQUIRE_NOTHROW(helper.HandlePost(L"https://testUri", {}));
    }
}

TEST_CASE("HttpClientHelper_PinningConfiguration", "[RestSource]")
{
    // Create the Store chain config
    PinningChain chain;
    auto chainElement = chain.Root();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_ROOT_1).SetPinning(PinningVerificationType::PublicKey);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_INTERMEDIATE_1).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);
    chainElement = chainElement.Next();
    chainElement->LoadCertificate(IDX_CERTIFICATE_STORE_LEAF_1).SetPinning(PinningVerificationType::Subject | PinningVerificationType::Issuer);

    PinningConfiguration config;
    config.AddChain(chain);

    HttpClientHelper helper;
    helper.SetPinningConfiguration(config);

    REQUIRE_THROWS_HR(helper.HandleGet(L"https://microsoft.com"), APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH);
}
