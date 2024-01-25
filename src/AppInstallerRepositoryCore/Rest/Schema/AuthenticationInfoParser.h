// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include <winget/Authentication.h>
#include <winget/ManifestCommon.h>

namespace AppInstaller::Repository::Rest::Schema
{
    Authentication::AuthenticationInfo ParseAuthenticationInfo(const web::json::value& dataObject, std::optional<Manifest::ManifestVer> manifestVersion = {});
}
