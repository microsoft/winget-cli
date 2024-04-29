// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Authentication.h>
#include <winget/JsonUtil.h>
#include <AppInstallerArchitecture.h>

#include <string>
#include <optional>
#include <string_view>
#include <memory>
#include <vector>

namespace AppInstaller::MSStore
{
    struct MSStoreDownloadFile
    {
        std::string Url;
        std::vector<BYTE> Sha256;
        std::string FileName;
    };

    struct MSStoreDownloadInfo
    {
        std::vector<MSStoreDownloadFile> MainPackages;
        std::vector<MSStoreDownloadFile> DependencyPackages;
    };

    struct MSStoreDownloadContext
    {
        MSStoreDownloadContext(std::string productId, AppInstaller::Utility::Architecture architecture, std::string locale, AppInstaller::Authentication::AuthenticationArguments authArgs) {};

        // Calls display catalog API and sfs-client to get download info.
        MSStoreDownloadInfo GetDwonloadInfo();

        // Gets license for the corresponding package returned by previous GetDwonloadInfo().
        // GetDwonloadInfo() must be called before calling this method.
        std::vector<BYTE> GetLicense();

    private:
        std::string m_productId;
        std::vector<AppInstaller::Utility::Architecture> m_architectures;
        std::vector<std::string> m_locales;
        AppInstaller::Authentication::Authenticator m_licensingAuthenticator;
        std::string m_wuCategoryId;
        std::string m_contentId;
    };

    enum class DisplayCatalogPackageFormatEnum
    {
        Unknown,
        AppxBundle,
        MsixBundle,
        Appx,
        Msix,
    };

    struct DisplayCatalogPackage
    {
        std::string PackageId;

        std::vector<AppInstaller::Utility::Architecture> Architectures;

        std::vector<std::string> Languages;

        DisplayCatalogPackageFormatEnum PackageFormat = DisplayCatalogPackageFormatEnum::Unknown;

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

            virtual bool IsApplicable(const DisplayCatalogPackage& package) = 0;

            virtual bool IsFirstBetter(const DisplayCatalogPackage& first, const MSStore::DisplayCatalogPackage& second) = 0;

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
