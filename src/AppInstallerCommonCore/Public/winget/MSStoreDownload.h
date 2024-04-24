// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/JsonUtil.h>
#include <AppInstallerArchitecture.h>
#include <string>

using namespace AppInstaller::Utility;

namespace AppInstaller::MSStore
{
    enum class PackageFormatEnum
    {
        Unknown = -1,
        AppxBundle,
        EAppxBundle,
        MsixBundle,
        Appx,
        Msix,
    };

    struct MSStoreDisplayCatalogPackage
    {
        std::vector<Architecture> Architectures;

        std::vector<std::string> Languages;

        PackageFormatEnum PackageFormat = PackageFormatEnum::Unknown;

        std::string WuCategoryId;

        std::string ContentId;
    };

    // Deserializes the display catalog packages from the provided json object. 
    std::vector<MSStoreDisplayCatalogPackage> DeserializeDisplayCatalogPackages(const web::json::value& jsonObject);

    // Constructs the MSStore catalog rest api with the provided product id and language.
    std::string GetStoreCatalogRestApi(const std::string& productId, const std::string& language);

    PackageFormatEnum ConvertToPackageFormatEnum(std::string_view packageFormatStr);
}
