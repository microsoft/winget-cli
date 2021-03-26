// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestHelper.h"

namespace AppInstaller::Repository::Rest::Schema
{
    std::string RestHelper::GetRestAPIBaseUri(std::string restApiUri)
    {
        if (!restApiUri.empty() && restApiUri.back() == '/')
        {
            restApiUri.pop_back();
        }

        return restApiUri;
    }
}
