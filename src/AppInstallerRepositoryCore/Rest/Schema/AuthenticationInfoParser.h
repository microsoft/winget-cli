// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/json.h>
#include <winget/Authentication.h>
#include <winget/ManifestCommon.h>

namespace AppInstaller::Repository::Rest::Schema
{
    enum class ParseAuthenticationInfoType
    {
        Source,
        Installer,
    };

    // Parses AuthenticationInfo from json object.
    // This could be used for installer level parsing as well in manifest deserializer (currently not supported, manifestVersion not used).
    // The authentication info json looks like below:
    // "Authentication": {
    //     "AuthenticationType": "microsoftEntraId",
    //     "MicrosoftEntraIdAuthenticationInfo" : {
    //         "Resource": "GUID",
    //         "Scope" : "test"
    //     }
    // }
    Authentication::AuthenticationInfo ParseAuthenticationInfo(const web::json::value& dataObject, ParseAuthenticationInfoType parseType, std::optional<Manifest::ManifestVer> manifestVersion = {});
}
