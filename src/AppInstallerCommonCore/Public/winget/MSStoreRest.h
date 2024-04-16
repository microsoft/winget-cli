// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/JsonUtil.h>
#include <string>

namespace AppInstaller::MSStore
{
    // Move this out to a separate file so that we don't have to include it in the another file.
    struct MSStoreRestHelper
    {
        static std::string GetWuCategoryId(const std::string& productId, const std::string& language, const std::string& marketplace);

    protected:
        static std::optional<std::string> GetWuCategoryIdInternal(const web::json::value& dataObject);
    };
}
