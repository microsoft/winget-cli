// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/JsonUtil.h>
#include <AppInstallerArchitecture.h>

#include <string>
#include <optional>
#include <string_view>
#include <memory>
#include <vector>

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

    struct MSStoreCatalogPackage
    {
        std::string PackageId;

        std::vector<AppInstaller::Utility::Architecture> Architectures;

        std::vector<std::string> Languages;

        PackageFormatEnum PackageFormat = PackageFormatEnum::Unknown;

        std::string WuCategoryId;

        std::string ContentId;
    };

    namespace details
    {
        struct MSStoreCatalogPackageComparisonField
        {
            MSStoreCatalogPackageComparisonField(std::string_view name) : m_name(name) {}

            virtual ~MSStoreCatalogPackageComparisonField() = default;

            std::string_view Name() const { return m_name; }

            virtual bool IsApplicable(const MSStoreCatalogPackage& package) = 0;

            virtual bool IsFirstBetter(const MSStoreCatalogPackage& first, const MSStore::MSStoreCatalogPackage& second) = 0;

        private:
            std::string_view m_name;
        };
    }

    // Class for comparing MSStore packages.
    struct DisplayCatalogPackageComparator
    {
        DisplayCatalogPackageComparator(const std::vector<std::string>& requiredLanguages, const std::vector<AppInstaller::Utility::Architecture>& requiredArchs);

        // Gets the best installer from the manifest, if at least one is applicable.
        std::optional<MSStoreCatalogPackage> GetPreferredPackage(const std::vector<MSStoreCatalogPackage>& package);

        // Determines if an installer is applicable.
        bool IsApplicable(const MSStoreCatalogPackage& package);

        //// Determines if the first installer is a better choice.
        bool IsFirstBetter(const MSStoreCatalogPackage& first, const MSStoreCatalogPackage& second);

    private:
        void AddComparator(std::unique_ptr<details::MSStoreCatalogPackageComparisonField>&& comparator);

        std::vector<std::unique_ptr<details::MSStoreCatalogPackageComparisonField>> m_comparators;
    };

    // Deserializes the display catalog packages from the provided json object. 
    std::vector<MSStoreCatalogPackage> DeserializeMSStoreCatalogPackages(const web::json::value& jsonObject);

    // Constructs the MSStore catalog rest api with the provided product id and language.
    std::string GetMSStoreCatalogRestApi(const std::string& productId, const std::string& language);

    // Coverts the package format string to the corresponding PackageFormatEnum.
    PackageFormatEnum ConvertToPackageFormatEnum(std::string_view packageFormatStr);
}
