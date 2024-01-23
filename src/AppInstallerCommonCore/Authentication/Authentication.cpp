// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Authentication.h"

namespace AppInstaller::Authentication
{
    namespace
    {
        const std::string c_BearerTokenPrefix = "Bearer ";
    }

    std::string AppInstaller::Authentication::CreateBearerToken(std::string rawToken)
    {
        return c_BearerTokenPrefix + rawToken;
    }
}
