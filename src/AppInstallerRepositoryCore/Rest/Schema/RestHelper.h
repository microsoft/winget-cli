// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Repository::Rest::Schema
{
    // Rest source helper.
    struct RestHelper
    {
        static std::string GetRestAPIBaseUri(std::string restApiUri);
    };
}
