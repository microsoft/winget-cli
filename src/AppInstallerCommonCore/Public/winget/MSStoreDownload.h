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

    enum class PackageInapplicabilityFlags : int
    {
        None = 0x0,
        PackageFormat = 0x1,
        Language = 0x2,
        Architecture = 0x4,
    };

    struct DisplayCatalogPackageComparator
    {
        DisplayCatalogPackageComparator(const Execution::Context& context);


    };

    // Class in charge of comparing manifest entries
    struct DisplayCatalogPackageComparator
    {
        DisplayCatalogPackageComparator(const Execution::Context& context);

        // Gets the best installer from the manifest, if at least one is applicable.
        PackageAndInapplicabilities GetPreferredPackage(const std::vector<MSStoreDisplayCatalogPackage>& package);

        // Determines if an installer is applicable.
        PackageInapplicabilityFlags IsApplicable(const MSStoreDisplayCatalogPackage& package);

        // Determines if the first installer is a better choice.
        bool IsFirstBetter(
            const MSStoreDisplayCatalogPackage& first,
            const MSStoreDisplayCatalogPackage& second);

    private:
        void AddComparator(std::unique_ptr<details::ComparisonField>&& comparator);
        std::vector<details::ComparisonField*> m_comparators;
    };
}
