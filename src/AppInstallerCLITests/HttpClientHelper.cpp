// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHandler.h"
#include "AppInstallerErrors.h"
#include <Rest/HttpClientHelper.h>

using namespace AppInstaller::Repository::Rest;

TEST_CASE("ExtractJsonResponse", "[RestSource]")
{
    HttpClientHelper helper{ GetTestHandler(web::http::status_codes::OK, L"test response", web::http::details::mime_types::text_plain) };
    REQUIRE_THROWS_HR(helper.HandleGet(L"https://testUri"), APPINSTALLER_CLI_ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE);
}

TEST_CASE("ValidateAndExtractResponse", "[RestSource]")
{
    HttpClientHelper helper{ GetTestHandler(web::http::status_codes::ServiceUnavailable) };
    REQUIRE_THROWS(helper.HandleGet(L"https://testUri"));
}
