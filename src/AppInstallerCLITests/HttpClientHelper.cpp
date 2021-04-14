// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <AppInstallerErrors.h>
#include <Rest/HttpClientHelper.h>

using namespace AppInstaller::Repository::Rest;

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
