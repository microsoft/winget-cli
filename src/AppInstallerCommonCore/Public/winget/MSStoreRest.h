// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/JsonUtil.h>
#include <string>

namespace AppInstaller::MSStore
{
    // Constructs the MSStore catalog rest api with the provided product id and language.
    std::string GetStoreCatalogRestApi(const std::string& productId, const std::string& language);

    // Retrieves the corresponding WuCategoryId from the json response object;
    std::string GetWuCategoryId(const web::json::value& jsonObject);
}
