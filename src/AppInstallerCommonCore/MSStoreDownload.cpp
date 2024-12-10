// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <AppinstallerLogging.h>
#include "AppInstallerMsixInfo.h"
#include "AppInstallerRuntime.h"
#include "winget/HttpClientHelper.h"
#include "winget/JsonUtil.h"
#include "winget/Locale.h"
#include "winget/MSStoreDownload.h"
#include "winget/NetworkSettings.h"
#include "winget/Rest.h"
#include "winget/UserSettings.h"
#ifndef WINGET_DISABLE_FOR_FUZZING
#include <sfsclient/SFSClient.h>
#endif

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;

#ifndef AICLI_DISABLE_TEST_HOOKS
    namespace TestHooks
    {
        static std::shared_ptr<web::http::http_pipeline_stage> s_DisplayCatalog_HttpPipelineStage_Override = nullptr;

        void SetDisplayCatalogHttpPipelineStage_Override(std::shared_ptr<web::http::http_pipeline_stage> value)
        {
            s_DisplayCatalog_HttpPipelineStage_Override = value;
        }

        static std::function<std::vector<SFS::AppContent>(std::string_view)>* s_SfsClient_AppContents_Override = nullptr;

        void SetSfsClientAppContents_Override(std::function<std::vector<SFS::AppContent>(std::string_view)>* value)
        {
            s_SfsClient_AppContents_Override = value;
        }

        static std::shared_ptr<web::http::http_pipeline_stage> s_Licensing_HttpPipelineStage_Override = nullptr;

        void SetLicensingHttpPipelineStage_Override(std::shared_ptr<web::http::http_pipeline_stage> value)
        {
            s_Licensing_HttpPipelineStage_Override = value;
        }
    }
#endif

    namespace DisplayCatalogDetails
    {
        // Default preferred sku to use
        constexpr std::string_view TargetSkuIdValue = "0015"sv;

        // Json response fields
        constexpr std::string_view Product = "Product"sv;
        constexpr std::string_view DisplaySkuAvailabilities = "DisplaySkuAvailabilities"sv;
        constexpr std::string_view Sku = "Sku"sv;
        constexpr std::string_view SkuId = "SkuId"sv;
        constexpr std::string_view Properties = "Properties"sv;
        constexpr std::string_view Packages = "Packages"sv;
        constexpr std::string_view Languages = "Languages"sv;
        constexpr std::string_view PackageFormat = "PackageFormat"sv;
        constexpr std::string_view PackageId = "PackageId"sv;
        constexpr std::string_view Architectures = "Architectures"sv;
        constexpr std::string_view ContentId = "ContentId"sv;
        constexpr std::string_view FulfillmentData = "FulfillmentData"sv;
        constexpr std::string_view WuCategoryId = "WuCategoryId"sv;

        // Display catalog rest endpoint
        constexpr std::string_view DisplayCatalogRestApi = R"(https://displaycatalog.mp.microsoft.com/v7.0/products/{0}?fieldsTemplate={1}&market={2}&languages={3}&catalogIds={4})";
        constexpr std::string_view Details = "Details"sv;
        constexpr std::string_view Neutral = "Neutral"sv;
        constexpr std::string_view TargetCatalogId = "4"sv;

        enum class DisplayCatalogPackageFormatEnum
        {
            Unknown,
            AppxBundle,
            MsixBundle,
            Appx,
            Msix,
        };

        DisplayCatalogPackageFormatEnum ConvertToPackageFormatEnum(std::string_view packageFormatStr)
        {
            std::string packageFormat = Utility::ToLower(packageFormatStr);
            if (packageFormat == "appxbundle")
            {
                return DisplayCatalogPackageFormatEnum::AppxBundle;
            }
            else if (packageFormat == "msixbundle")
            {
                return DisplayCatalogPackageFormatEnum::MsixBundle;
            }
            else if (packageFormat == "appx")
            {
                return DisplayCatalogPackageFormatEnum::Appx;
            }
            else if (packageFormat == "msix")
            {
                return DisplayCatalogPackageFormatEnum::Msix;
            }

            AICLI_LOG(Core, Info, << "ConvertToPackageFormatEnum: Unknown package format: " << packageFormatStr);
            return DisplayCatalogPackageFormatEnum::Unknown;
        }

        struct DisplayCatalogPackage
        {
            std::string PackageId;

            std::vector<AppInstaller::Utility::Architecture> Architectures;

            std::vector<std::string> Languages;

            DisplayCatalogPackageFormatEnum PackageFormat = DisplayCatalogPackageFormatEnum::Unknown;

            // To be used later in sfs-client
            std::string WuCategoryId;

            // To be used later in licensing
            std::string ContentId;
        };

        // Display catalog package comparison logic.
        // The comparator follows similar logic as ManifestComparator.
        namespace DisplayCatalogPackageComparison
        {
            struct DisplayCatalogPackageComparisonField
            {
                DisplayCatalogPackageComparisonField(std::string_view name) : m_name(name) {}

                virtual ~DisplayCatalogPackageComparisonField() = default;

                std::string_view Name() const { return m_name; }

                virtual bool IsApplicable(const DisplayCatalogPackage& package) = 0;

                virtual bool IsFirstBetter(const DisplayCatalogPackage& first, const DisplayCatalogPackage& second) = 0;

            private:
                std::string_view m_name;
            };

            struct PackageFormatComparator : public DisplayCatalogPackageComparisonField
            {
                PackageFormatComparator() : DisplayCatalogPackageComparisonField("Package Format") {}

                bool IsApplicable(const DisplayCatalogPackage& package) override
                {
                    return package.PackageFormat != DisplayCatalogPackageFormatEnum::Unknown;
                }

                bool IsFirstBetter(const DisplayCatalogPackage& first, const DisplayCatalogPackage& second) override
                {
                    return IsPackageFormatBundle(first) && !IsPackageFormatBundle(second);
                }

            private:
                bool IsPackageFormatBundle(const DisplayCatalogPackage& package)
                {
                    return
                        package.PackageFormat == DisplayCatalogPackageFormatEnum::AppxBundle ||
                        package.PackageFormat == DisplayCatalogPackageFormatEnum::MsixBundle;
                }
            };

            struct LocaleComparator : public DisplayCatalogPackageComparisonField
            {
                LocaleComparator(std::string locale) : DisplayCatalogPackageComparisonField("Locale")
                {
                    if (!locale.empty())
                    {
                        m_locales.emplace_back(std::move(locale));
                        m_isRequirement = true;
                    }
                    else
                    {
                        m_locales = Locale::GetUserPreferredLanguages();
                    }

                    AICLI_LOG(Core, Verbose,
                        << "Locale Comparator created with locales: " << Utility::ConvertContainerToString(m_locales)
                        << " , Is requirement: " << m_isRequirement);
                }

                bool IsApplicable(const DisplayCatalogPackage& package) override
                {
                    if (m_isRequirement)
                    {
                        for (auto const& locale : m_locales)
                        {
                            double distanceScore = GetBestDistanceScoreFromList(locale, package.Languages);
                            if (distanceScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                            {
                                return true;
                            }
                        }

                        return false;
                    }
                    else
                    {
                        return true;
                    }
                }

                bool IsFirstBetter(const DisplayCatalogPackage& first, const DisplayCatalogPackage& second)
                {
                    for (auto const& locale : m_locales)
                    {
                        double firstScore = GetBestDistanceScoreFromList(locale, first.Languages);
                        double secondScore = GetBestDistanceScoreFromList(locale, second.Languages);

                        if (firstScore >= Locale::MinimumDistanceScoreAsCompatibleMatch || secondScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                        {
                            return firstScore > secondScore;
                        }
                    }

                    return false;
                }

            private:
                double GetBestDistanceScoreFromList(std::string_view targetLocale, const std::vector<std::string>& locales)
                {
                    double finalScore = 0;
                    for (auto const& locale : locales)
                    {
                        double currentScore = Locale::GetDistanceOfLanguage(targetLocale, locale);
                        if (currentScore > finalScore)
                        {
                            finalScore = currentScore;
                        }
                    }

                    return finalScore;
                }

                std::vector<std::string> m_locales;
                bool m_isRequirement = false;
            };

            struct ArchitectureComparator : public DisplayCatalogPackageComparisonField
            {
                ArchitectureComparator(Utility::Architecture architecture) : DisplayCatalogPackageComparisonField("Architecture")
                {
                    if (architecture != Utility::Architecture::Unknown)
                    {
                        m_architectures.emplace_back(architecture);
                        m_isRequirement = true;
                    }
                    else
                    {
                        m_architectures = Utility::GetApplicableArchitectures();
                    }

                    AICLI_LOG(Core, Verbose,
                        << "Architecture Comparator created with archs: " << Utility::ConvertContainerToString(m_architectures, Utility::ToString)
                        << " , Is requirement: " << m_isRequirement);
                }

                bool IsApplicable(const DisplayCatalogPackage& package) override
                {
                    if (m_isRequirement)
                    {
                        for (auto arch : package.Architectures)
                        {
                            if (Utility::IsApplicableArchitecture(arch, m_architectures) > Utility::InapplicableArchitecture)
                            {
                                return true;
                            }
                        }

                        return false;
                    }
                    else
                    {
                        return true;
                    }
                }

                bool IsFirstBetter(const DisplayCatalogPackage& first, const DisplayCatalogPackage& second) override
                {
                    for (auto arch : m_architectures)
                    {
                        auto firstItr = std::find(first.Architectures.begin(), first.Architectures.end(), arch);
                        auto secondItr = std::find(second.Architectures.begin(), second.Architectures.end(), arch);

                        if (firstItr != first.Architectures.end() && secondItr == second.Architectures.end())
                        {
                            true;
                        }
                        else if (secondItr != second.Architectures.end())
                        {
                            return false;
                        }
                    }

                    return false;
                }

            private:
                std::vector<Utility::Architecture> m_architectures;
                bool m_isRequirement = false;
            };

            struct DisplayCatalogPackageComparator
            {
                DisplayCatalogPackageComparator(std::string requiredLocale, Utility::Architecture requiredArch)
                {
                    // Order of comparators matters.
                    AddComparator(std::make_unique<LocaleComparator>(requiredLocale));
                    AddComparator(std::make_unique<ArchitectureComparator>(requiredArch));
                    AddComparator(std::make_unique<PackageFormatComparator>());
                }

                // Gets the best installer from the manifest, if at least one is applicable.
                std::optional<DisplayCatalogPackage> GetPreferredPackage(const std::vector<DisplayCatalogPackage>& packages)
                {
                    AICLI_LOG(Core, Verbose, << "Starting display catalog package selection.");

                    const DisplayCatalogPackage* result = nullptr;
                    for (const auto& package : packages)
                    {
                        if (IsApplicable(package) && (!result || IsFirstBetter(package, *result)))
                        {
                            result = &package;
                        }
                    }

                    if (result)
                    {
                        return *result;
                    }
                    else
                    {
                        return {};
                    }
                }

                // Determines if the package is applicable.
                bool IsApplicable(const DisplayCatalogPackage& package)
                {
                    for (const auto& comparator : m_comparators)
                    {
                        if (!comparator->IsApplicable(package))
                        {
                            return false;
                        }
                    }

                    return true;
                }

                // Determines if the first package is a better choice.
                bool IsFirstBetter(const DisplayCatalogPackage& first, const DisplayCatalogPackage& second)
                {
                    for (const auto& comparator : m_comparators)
                    {
                        bool forwardCompare = comparator->IsFirstBetter(first, second);
                        bool reverseCompare = comparator->IsFirstBetter(second, first);

                        if (forwardCompare && reverseCompare)
                        {
                            AICLI_LOG(Core, Error, << "Packages are both better than each other?");
                            THROW_HR(E_UNEXPECTED);
                        }

                        if (forwardCompare && !reverseCompare)
                        {
                            AICLI_LOG(Core, Verbose, << "Package " << first.PackageId << " is better than " << second.PackageId);
                            return true;
                        }
                    }

                    AICLI_LOG(Core, Verbose, << "Package " << first.PackageId << " is equivalent in priority to " << second.PackageId);
                    return false;
                }

            private:
                void AddComparator(std::unique_ptr<DisplayCatalogPackageComparisonField>&& comparator)
                {
                    if (comparator)
                    {
                        m_comparators.emplace_back(std::move(comparator));
                    }
                }

                std::vector<std::unique_ptr<DisplayCatalogPackageComparisonField>> m_comparators;
            };
        }

        // Display catalog API invocation and handling

        utility::string_t GetDisplayCatalogRestApi(std::string_view productId, std::string_view locale)
        {
            std::vector<Utility::LocIndString> locales;
            if (!locale.empty())
            {
                locales.emplace_back(locale);
            }
            else
            {
                for (auto const& localeEntry : Locale::GetUserPreferredLanguages())
                {
                    locales.emplace_back(localeEntry);
                }
            }

            // Neutral is always added
            locales.emplace_back(Neutral);

            auto restEndpoint = AppInstaller::Utility::Format(std::string{ DisplayCatalogRestApi },
                productId, Details, AppInstaller::Runtime::GetOSRegion(), Utility::Join(Utility::LocIndView(","), locales), TargetCatalogId);

            return JSON::GetUtilityString(restEndpoint);
        }

        // Response format:
        // {
        //   "Product": {
        //     "DisplaySkuAvailabilities": [
        //       {
        //         "Sku": {
        //           "SkuId": "0015",
        //           ... Sku Contents ...
        //         }
        //       }
        //     ]
        //   }
        // }
        std::reference_wrapper<const web::json::value> GetSkuNodeFromDisplayCatalogResponse(const web::json::value& responseObject)
        {
            AICLI_LOG(Core, Info, << "Started parsing display catalog response. Try to find target sku: " << TargetSkuIdValue);

            if (responseObject.is_null())
            {
                AICLI_LOG(Core, Error, << "Missing DisplayCatalog Response json object.");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            std::optional<std::reference_wrapper<const web::json::value>> product = JSON::GetJsonValueFromNode(responseObject, JSON::GetUtilityString(Product));
            if (!product)
            {
                AICLI_LOG(Core, Error, << "Missing Product node");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            auto skuEntries = JSON::GetRawJsonArrayFromJsonNode(product.value().get(), JSON::GetUtilityString(DisplaySkuAvailabilities));
            if (!skuEntries)
            {
                AICLI_LOG(Core, Error, << "Missing DisplaySkuAvailabilities");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            for (const auto& skuEntry : skuEntries.value().get())
            {
                std::optional<std::reference_wrapper<const web::json::value>> sku = JSON::GetJsonValueFromNode(skuEntry, JSON::GetUtilityString(Sku));
                if (!sku)
                {
                    AICLI_LOG(Core, Error, << "Missing Sku");
                    THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
                }

                const auto& skuValue = sku.value().get();
                auto skuId = JSON::GetRawStringValueFromJsonNode(skuValue, JSON::GetUtilityString(SkuId)).value_or("");
                if (TargetSkuIdValue == skuId)
                {
                    AICLI_LOG(Core, Info, << "Target Sku (" << TargetSkuIdValue << ") found");
                    return skuValue;
                }
            }

            AICLI_LOG(Core, Error, << "Target Sku (" << TargetSkuIdValue << ") not found");
            THROW_HR(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE);
        }

        // Response format:
        // {
        //   "Sku": {
        //     "Properties": {
        //       "Packages": [
        //         {
        //           "PackageId": "package id",
        //           "Architectures": [ "x86", "x64" ],
        //           "Languages": [ "en", "fr" ],
        //           "PackageFormat": "AppxBundle",
        //           "ContentId": "guid",
        //           "FulfillmentData": {
        //             "WuCategoryId": "guid",
        //           }
        //         }
        //       ]
        //     }
        //   }
        // }
        std::vector<DisplayCatalogPackage> GetDisplayCatalogPackagesFromSkuNode(const web::json::value& jsonObject)
        {
            AICLI_LOG(Core, Info, << "Started extracting display catalog packages from sku.");

            std::optional<std::reference_wrapper<const web::json::value>> properties = JSON::GetJsonValueFromNode(jsonObject, JSON::GetUtilityString(Properties));
            if (!properties)
            {
                AICLI_LOG(Core, Error, << "Missing Properties");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            const auto& propertiesValue = properties.value().get();
            auto packages = JSON::GetRawJsonArrayFromJsonNode(propertiesValue, JSON::GetUtilityString(Packages));
            if (!packages)
            {
                AICLI_LOG(Core, Error, << "Missing Packages");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            std::vector<DisplayCatalogPackage> displayCatalogPackages;

            for (const auto& packageEntry : packages.value().get())
            {
                DisplayCatalogPackage catalogPackage;

                // Package Id
                catalogPackage.PackageId = JSON::GetRawStringValueFromJsonNode(packageEntry, JSON::GetUtilityString(PackageId)).value_or("");
                // Architectures
                auto architectures = JSON::GetRawStringArrayFromJsonNode(packageEntry, JSON::GetUtilityString(Architectures));
                for (const auto& arch : architectures)
                {
                    auto archEnum = Utility::ConvertToArchitectureEnum(arch);
                    if (archEnum != Utility::Architecture::Unknown)
                    {
                        catalogPackage.Architectures.emplace_back(archEnum);
                    }
                }
                // Languages
                auto languages = JSON::GetRawStringArrayFromJsonNode(packageEntry, JSON::GetUtilityString(Languages));
                for (const auto& language : languages)
                {
                    catalogPackage.Languages.emplace_back(language);
                }
                // Package Format
                auto packageFormat = JSON::GetRawStringValueFromJsonNode(packageEntry, JSON::GetUtilityString(PackageFormat)).value_or("");
                catalogPackage.PackageFormat = ConvertToPackageFormatEnum(packageFormat);
                // Content Id
                catalogPackage.ContentId = JSON::GetRawStringValueFromJsonNode(packageEntry, JSON::GetUtilityString(ContentId)).value_or("");
                if (catalogPackage.ContentId.empty())
                {
                    AICLI_LOG(Core, Warning, << "Missing ContentId");
                    // ContentId is required for licensing. Skip this package if missing.
                    continue;
                }
                // WuCategoryId
                std::optional<std::reference_wrapper<const web::json::value>> fulfillmentData = JSON::GetJsonValueFromNode(packageEntry, JSON::GetUtilityString(FulfillmentData));
                if (!fulfillmentData)
                {
                    AICLI_LOG(Core, Warning, << "Missing FulfillmentData");
                    // WuCategoryId is required for sfs-client. Skip this package if missing.
                    continue;
                }
                catalogPackage.WuCategoryId = JSON::GetRawStringValueFromJsonNode(fulfillmentData.value().get(), JSON::GetUtilityString(WuCategoryId)).value_or("");
                if (catalogPackage.WuCategoryId.empty())
                {
                    AICLI_LOG(Core, Warning, << "Missing WuCategoryId");
                    // WuCategoryId is required for sfs-client. Skip this package if missing.
                    continue;
                }

                displayCatalogPackages.emplace_back(std::move(catalogPackage));
            }

            return displayCatalogPackages;
        }

        DisplayCatalogPackage CallDisplayCatalogAndGetPreferredPackage(std::string_view productId, std::string_view locale, Utility::Architecture architecture, const Http::HttpClientHelper::HttpRequestHeaders& authHeaders)
        {
            AICLI_LOG(Core, Info, << "CallDisplayCatalogAndGetPreferredPackage with ProductId: " << productId << " Locale: " << locale << " Architecture: " << Utility::ToString(architecture));

            auto displayCatalogApi = GetDisplayCatalogRestApi(productId, locale);

            AppInstaller::Http::HttpClientHelper httpClientHelper;

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (TestHooks::s_DisplayCatalog_HttpPipelineStage_Override)
            {
                httpClientHelper = AppInstaller::Http::HttpClientHelper{ TestHooks::s_DisplayCatalog_HttpPipelineStage_Override };
            }
#endif

            std::optional<web::json::value> displayCatalogResponseObject = httpClientHelper.HandleGet(displayCatalogApi, {}, authHeaders);

            if (!displayCatalogResponseObject)
            {
                AICLI_LOG(Core, Error, << "No display catalog json object found");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            const auto& sku = GetSkuNodeFromDisplayCatalogResponse(displayCatalogResponseObject.value());
            auto displayCatalogPackages = GetDisplayCatalogPackagesFromSkuNode(sku.get());

            DisplayCatalogPackageComparison::DisplayCatalogPackageComparator packageComparator{ std::string{ locale }, architecture };
            auto preferredPackageResult = packageComparator.GetPreferredPackage(displayCatalogPackages);

            if (!preferredPackageResult)
            {
                AICLI_LOG(Core, Error,
                    << "No applicable display catalog package found for ProductId: " << productId
                    << " , Locale: " << locale << " , Architecture: " << Utility::ToString(architecture));

                THROW_HR(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE);
            }

            auto preferredPackage = preferredPackageResult.value();

            AICLI_LOG(Core, Info,
                << "DisplayCatalog package selected. WuCategoryId: " << preferredPackage.WuCategoryId
                << " , ContentId: " << preferredPackage.ContentId);

            return preferredPackage;
        }
    }

#ifndef WINGET_DISABLE_FOR_FUZZING
    namespace SfsClientDetails
    {
        const std::string SupportedFileTypes[] = { ".msix", ".msixbundle", ".appx", ".appxbundle" };

        Manifest::PlatformEnum ConvertFromSfsPlatform(std::string_view applicability)
        {
            if (Utility::CaseInsensitiveStartsWith(applicability, "universal"))
            {
                return Manifest::PlatformEnum::Universal;
            }
            else if (Utility::CaseInsensitiveStartsWith(applicability, "desktop"))
            {
                return Manifest::PlatformEnum::Desktop;
            }
            else if (Utility::CaseInsensitiveStartsWith(applicability, "iot"))
            {
                return Manifest::PlatformEnum::IoT;
            }
            else if (Utility::CaseInsensitiveStartsWith(applicability, "analog"))
            {
                return Manifest::PlatformEnum::Holographic;
            }
            else if (Utility::CaseInsensitiveStartsWith(applicability, "ppi"))
            {
                return Manifest::PlatformEnum::Team;
            }

            return Manifest::PlatformEnum::Unknown;
        }

        Utility::Architecture ConvertFromSfsArchitecture(SFS::Architecture sfsArchitecture)
        {
            switch (sfsArchitecture)
            {
            case SFS::Architecture::Amd64:
                return Utility::Architecture::X64;
            case SFS::Architecture::x86:
                return Utility::Architecture::X86;
            case SFS::Architecture::Arm64:
                return Utility::Architecture::Arm64;
            case SFS::Architecture::Arm:
                return Utility::Architecture::Arm;
            case SFS::Architecture::None:
                return Utility::Architecture::Neutral;
            }

            return Utility::Architecture::Unknown;
        }

        std::vector<Manifest::PlatformEnum> GetSfsPackageFileSupportedPlatforms(const SFS::AppFile& appFile, Manifest::PlatformEnum requiredPlatform)
        {
            std::vector<Manifest::PlatformEnum> supportedPlatforms;

            for (auto const& applicability : appFile.GetApplicabilityDetails().GetPlatformApplicabilityForPackage())
            {
                auto platform = ConvertFromSfsPlatform(applicability);
                if (platform != Manifest::PlatformEnum::Unknown &&
                    (platform == requiredPlatform || requiredPlatform == Manifest::PlatformEnum::Unknown))
                {
                    supportedPlatforms.emplace_back(platform);
                }
            }

            return supportedPlatforms;
        }

        std::vector<Utility::Architecture> GetSfsPackageFileSupportedArchitectures(const SFS::AppFile& appFile, Utility::Architecture requiredArchitecture)
        {
            std::vector<Utility::Architecture> supportedArchitectures;

            for (auto const& sfsArchitecture : appFile.GetApplicabilityDetails().GetArchitectures())
            {
                auto convertedArchitecture = ConvertFromSfsArchitecture(sfsArchitecture);
                if (convertedArchitecture == Utility::Architecture::Unknown)
                {
                    continue;
                }

                if (requiredArchitecture == Utility::Architecture::Unknown || // No required architecture
                    convertedArchitecture == requiredArchitecture)
                {
                    supportedArchitectures.emplace_back(convertedArchitecture);
                }
            }

            return supportedArchitectures;
        }

        // This also checks if the file type is supported. If not supported, the return is empty string.
        std::string GetSfsPackageFileExtension(const SFS::AppFile& appFile)
        {
            std::string fileExtension = std::filesystem::path{ appFile.GetFileId() }.extension().u8string();

            bool fileTypeSupported = false;
            for (auto const& supportedFileType : SupportedFileTypes)
            {
                if (Utility::CaseInsensitiveEquals(supportedFileType, fileExtension))
                {
                    fileTypeSupported = true;
                    break;
                }
            }

            if (!fileTypeSupported)
            {
                return {};
            }

            return fileExtension;
        }

        // The file name will be {Name}_{Version}_{Platform list}_{Arch list}.{File Extension}
        // If the file name is longer than 256, file moniker will be used.
        std::string GetSfsPackageFileNameForDownload(
            const std::string& packageName,
            const Utility::UInt64Version& packageVersion,
            const std::vector<Manifest::PlatformEnum>& supportedPlatforms,
            const std::vector<Utility::Architecture>& supportedArchitectures,
            const std::string& fileExtension,
            const std::string& fileMoniker)
        {
            std::string platformString;
            for (auto platform : supportedPlatforms)
            {
                platformString += std::string{ Manifest::PlatformToString(platform, true) } + '.';
            }
            platformString.resize(platformString.size() - 1);

            std::string architectureString;
            for (auto architecture : supportedArchitectures)
            {
                architectureString += std::string{ Utility::ToString(architecture) } + '.';
            }
            architectureString.resize(architectureString.size() - 1);

            std::string fileName =
                packageName + '_' +
                packageVersion.ToString() + '_' +
                platformString + '_' +
                architectureString +
                fileExtension;

            if (fileName.length() < 256)
            {
                return fileName;
            }
            else
            {
                return fileMoniker + fileExtension;
            }
        }

        void SfsClientLoggingCallback(const SFS::LogData& logData)
        {
            std::string message = "Message: " + std::string{ logData.message };
            message += " File: " + std::string{ logData.file };
            message += " Line: " + std::to_string(logData.line);
            message += " Function: " + std::string{ logData.function };

            switch (logData.severity)
            {
            case SFS::LogSeverity::Verbose:
                AICLI_LOG(Core, Verbose, << message);
                break;
            case SFS::LogSeverity::Info:
                AICLI_LOG(Core, Info, << message);
                break;
            case SFS::LogSeverity::Warning:
                AICLI_LOG(Core, Warning, << message);
                break;
            case SFS::LogSeverity::Error:
                AICLI_LOG(Core, Error, << message);
                break;
            }
        }

        const std::unique_ptr<SFS::SFSClient>& GetSfsClientInstance()
        {
            static std::unique_ptr<SFS::SFSClient> s_sfsClient;
            static std::once_flag s_sfsClientInitializeOnce;

            std::call_once(s_sfsClientInitializeOnce,
                [&]()
                {
                    SFS::ClientConfig config;
                    config.accountId = "storeapps";
                    config.instanceId = "storeapps";
                    config.logCallbackFn = SfsClientLoggingCallback;

                    auto result = SFS::SFSClient::Make(config, s_sfsClient);
                    if (!result)
                    {
                        AICLI_LOG(Core, Error, << "Failed to initialize SfsClient. Error code: " << result.GetCode() << " Message: " << result.GetMsg());
                        THROW_HR_MSG(APPINSTALLER_CLI_ERROR_SFSCLIENT_API_FAILED, "Failed to initialize SfsClient. ErrorCode: %lu Message: %hs", result.GetCode(), result.GetMsg().c_str());
                    }
                });

            return s_sfsClient;
        }

        std::vector<MSStoreDownloadFile> PopulateSfsAppFileToMSStoreDownloadFileVector(
            const std::vector<SFS::AppFile>& appFiles,
            Utility::Architecture requiredArchitecture = Utility::Architecture::Unknown,
            Manifest::PlatformEnum requiredPlatform = Manifest::PlatformEnum::Unknown)
        {
            using PlatformAndArchitectureKey = std::pair<Manifest::PlatformEnum, Utility::Architecture>;

            // Since the server may return multiple versions of the same package, we'll use this map to record the one with latest version
            // for each Platform|Architecture pair.
            std::map<PlatformAndArchitectureKey, MSStoreDownloadFile> downloadFilesMap;

            for (auto const& appFile : appFiles)
            {
                // Filter out unsupported packages
                auto supportedPlatforms = GetSfsPackageFileSupportedPlatforms(appFile, requiredPlatform);
                if (supportedPlatforms.empty())
                {
                    AICLI_LOG(Core, Info, << "Package skipped due to unsupported platforms. FileId:" << appFile.GetFileId());
                    continue;
                }
                auto supportedArchitectures = GetSfsPackageFileSupportedArchitectures(appFile, requiredArchitecture);
                if (supportedArchitectures.empty())
                {
                    AICLI_LOG(Core, Info, << "Package skipped due to unsupported architecture. FileId:" << appFile.GetFileId());
                    continue;
                }
                std::string fileExtension = GetSfsPackageFileExtension(appFile);
                if (fileExtension.empty())
                {
                    AICLI_LOG(Core, Info, << "Package skipped due to unsupported file type. FileId:" << appFile.GetFileId());
                    continue;
                }

                MSStoreDownloadFile downloadFile;
                downloadFile.Url = appFile.GetUrl();
                // The sha256 hash was base64 encoded
                downloadFile.Sha256 = JSON::Base64Decode(appFile.GetHashes().at(SFS::HashType::Sha256));
                auto packageInfo = Msix::GetPackageIdInfoFromFullName(appFile.GetFileMoniker());
                downloadFile.Version = packageInfo.Version;
                downloadFile.FileName = GetSfsPackageFileNameForDownload(
                    packageInfo.Name, packageInfo.Version, supportedPlatforms,
                    supportedArchitectures, fileExtension, appFile.GetFileMoniker());

                // Update the platform architecture map with latest package if applicable
                for (auto supportedPlatform : supportedPlatforms)
                {
                    for (auto supportedArchitecture : supportedArchitectures)
                    {
                        PlatformAndArchitectureKey downloadFileKey{ supportedPlatform, supportedArchitecture };
                        if (downloadFile.Version > downloadFilesMap[downloadFileKey].Version)
                        {
                            downloadFilesMap[downloadFileKey] = downloadFile;
                        }
                    }
                }
            }

            // Generate MSStoreDownloadFile vector and remove duplication.
            std::vector<MSStoreDownloadFile> result;
            for (auto& downloadFileEntry : downloadFilesMap)
            {
                if (std::find_if(result.begin(), result.end(),
                    [&](const MSStoreDownloadFile& downloadFile)
                    {
                        return Utility::CaseInsensitiveEquals(downloadFile.FileName, downloadFileEntry.second.FileName);
                    }) == result.end())
                {
                    result.emplace_back(std::move(downloadFileEntry.second));
                }
            }

            return result;
        }

        MSStoreDownloadInfo CallSfsClientAndGetMSStoreDownloadInfo(std::string_view wuCategoryId, Utility::Architecture requiredArchitecture, Manifest::PlatformEnum requiredPlatform)
        {
            AICLI_LOG(Core, Info, << "CallSfsClientAndGetMSStoreDownloadInfo with WuCategoryId: " << wuCategoryId << " Architecture: " << Utility::ToString(requiredArchitecture) << " Platform: " << Manifest::PlatformToString(requiredPlatform));

            std::vector<SFS::AppContent> appContents;

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (TestHooks::s_SfsClient_AppContents_Override)
            {
                appContents = (*TestHooks::s_SfsClient_AppContents_Override)(wuCategoryId);
            }
            else
#endif
            {
                SFS::RequestParams sfsClientRequest;
                sfsClientRequest.productRequests = { {std::string{ wuCategoryId }, {}} };
                const auto& proxyUri = AppInstaller::Settings::Network().GetProxyUri();
                if (proxyUri)
                {
                    AICLI_LOG(Core, Info, << "Passing proxy to SFS client " << *proxyUri);
                    sfsClientRequest.proxy = *proxyUri;
                }

                auto requestResult = GetSfsClientInstance()->GetLatestAppDownloadInfo(sfsClientRequest, appContents);
                if (!requestResult)
                {
                    if (requestResult.GetCode() == SFS::Result::Code::HttpNotFound)
                    {
                        AICLI_LOG(Core, Error, << "Failed to call SfsClient GetLatestAppDownloadInfo. Package not found.");
                        THROW_HR_MSG(APPINSTALLER_CLI_ERROR_SFSCLIENT_PACKAGE_NOT_SUPPORTED, "Failed to call SfsClient GetLatestAppDownloadInfo. Package download not supported.");
                    }
                    else
                    {
                        AICLI_LOG(Core, Error, << "Failed to call SfsClient GetLatestAppDownloadInfo. Error code: " << requestResult.GetCode() << " Message: " << requestResult.GetMsg());
                        THROW_HR_MSG(APPINSTALLER_CLI_ERROR_SFSCLIENT_API_FAILED, "Failed to call SfsClient GetLatestAppDownloadInfo. ErrorCode: %lu Message: %hs", requestResult.GetCode(), requestResult.GetMsg().c_str());
                    }
                }
            }

            THROW_HR_IF(E_UNEXPECTED, appContents.empty());

            MSStoreDownloadInfo result;
            // Currently for app downloads, the result vector is always size 1.
            const auto& appContent = appContents.at(0);

            // Populate main packages
            result.MainPackages = PopulateSfsAppFileToMSStoreDownloadFileVector(appContent.GetFiles(), requiredArchitecture, requiredPlatform);

            // Populate dependency packages
            for (auto const& dependencyEntry : appContent.GetPrerequisites())
            {
                // Not passing in required platform for dependencies. Dependencies are mostly Windows.Universal.
                auto dependencyPackages = PopulateSfsAppFileToMSStoreDownloadFileVector(dependencyEntry.GetFiles(), requiredArchitecture);
                std::move(dependencyPackages.begin(), dependencyPackages.end(), std::inserter(result.DependencyPackages, result.DependencyPackages.end()));
            }

            if (result.MainPackages.empty())
            {
                AICLI_LOG(Core, Error, << "No applicable SFS main package.");
                THROW_HR(APPINSTALLER_CLI_ERROR_NO_APPLICABLE_SFSCLIENT_PACKAGE);
            }

            return result;
        }
    }
#endif

    namespace LicensingDetails
    {
        // Json response fields
        constexpr std::string_view License = "license"sv;
        constexpr std::string_view Keys = "keys"sv;
        constexpr std::string_view Value = "value"sv;

        // Licensing rest endpoint
        constexpr std::string_view LicensingRestEndpoint = "https://licensing.md.mp.microsoft.com/v9.0/licenses/offlineContent";
        constexpr std::string_view ContentId = "contentId"sv;
        constexpr std::string_view From = "From"sv;

        // Response:
        // {
        //   "license": {
        //     "keys": [ // returned as array for future, for now only 1 key
        //       {
        //         "value": "base64 encoded string"
        //       }
        //     ]
        //   }
        // }
        std::vector<BYTE> GetLicensing(std::string_view contentId, const Http::HttpClientHelper::HttpRequestHeaders& authHeaders)
        {
            AICLI_LOG(Core, Error, << "GetLicensing with ContentId: " << contentId);

            AppInstaller::Http::HttpClientHelper httpClientHelper;

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (TestHooks::s_Licensing_HttpPipelineStage_Override)
            {
                httpClientHelper = AppInstaller::Http::HttpClientHelper{ TestHooks::s_Licensing_HttpPipelineStage_Override };
            }
#endif

            web::json::value requestBody;
            requestBody[JSON::GetUtilityString(ContentId)] = web::json::value::string(JSON::GetUtilityString(contentId));
            Http::HttpClientHelper::HttpRequestHeaders requestHeaders;
            requestHeaders.insert_or_assign(JSON::GetUtilityString(From), L"winget-cli");

            std::optional<web::json::value> licensingResponseObject = std::nullopt;
            try
            {
                licensingResponseObject = httpClientHelper.HandlePost(
                    JSON::GetUtilityString(LicensingRestEndpoint), requestBody, requestHeaders, authHeaders);
            }
            catch (const wil::ResultException& re)
            {
                if (re.GetErrorCode() == HTTP_E_STATUS_FORBIDDEN)
                {
                    AICLI_LOG(CLI, Error, << "Getting MSStore package license failed. The Microsoft Entra Id account does not have privilege.");
                    THROW_HR(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED_FORBIDDEN);
                }
                else
                {
                    AICLI_LOG(CLI, Error, << "Getting MSStore package license failed. Error code: " << re.GetErrorCode());
                    THROW_HR(re.GetErrorCode());
                }
            }

            if (!licensingResponseObject || licensingResponseObject->is_null())
            {
                AICLI_LOG(Core, Error, << "Empty licensing response");
                THROW_HR(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED);
            }

            std::optional<std::reference_wrapper<const web::json::value>> license = JSON::GetJsonValueFromNode(licensingResponseObject.value(), JSON::GetUtilityString(License));
            if (!license)
            {
                AICLI_LOG(Core, Error, << "Missing license node");
                THROW_HR(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED);
            }

            auto keys = JSON::GetRawJsonArrayFromJsonNode(license.value().get(), JSON::GetUtilityString(Keys));
            if (!keys || keys->get().size() == 0)
            {
                AICLI_LOG(Core, Error, << "Missing keys or empty keys");
                THROW_HR(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED);
            }

            std::string base64LicenseContent = JSON::GetRawStringValueFromJsonNode(keys->get().at(0), JSON::GetUtilityString(Value)).value_or("");
            if (base64LicenseContent.empty())
            {
                AICLI_LOG(Core, Error, << "Missing license content");
                THROW_HR(APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED);
            }

            return JSON::Base64Decode(base64LicenseContent);
        }
    }

    namespace
    {
        Http::HttpClientHelper::HttpRequestHeaders GetAuthHeaders(std::unique_ptr<Authentication::Authenticator>& authenticator)
        {
            if (!authenticator)
            {
                return {};
            }

            Http::HttpClientHelper::HttpRequestHeaders result;

            auto authResult = authenticator->AuthenticateForToken();
            if (FAILED(authResult.Status))
            {
                AICLI_LOG(Repo, Error, << "Authentication failed. Result: " << authResult.Status);
                THROW_HR_MSG(authResult.Status, "Failed to authenticate for MicrosoftEntraId");
            }
            result.insert_or_assign(web::http::header_names::authorization, JSON::GetUtilityString(Authentication::CreateBearerToken(authResult.Token)));

            return result;
        }
    }

    MSStoreDownloadContext::MSStoreDownloadContext(
        std::string productId,
        AppInstaller::Utility::Architecture architecture,
        Manifest::PlatformEnum platform,
        std::string locale,
        AppInstaller::Authentication::AuthenticationArguments authArgs) :
        m_productId(std::move(productId)), m_architecture(architecture), m_platform(platform), m_locale(std::move(locale))
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (!TestHooks::s_DisplayCatalog_HttpPipelineStage_Override)
#endif
        {
            Authentication::MicrosoftEntraIdAuthenticationInfo displayCatalogMicrosoftEntraIdAuthInfo;
            displayCatalogMicrosoftEntraIdAuthInfo.Resource = "https://bigcatalog.commerce.microsoft.com";
            Authentication::AuthenticationInfo displayCatalogAuthInfo;
            displayCatalogAuthInfo.Type = Authentication::AuthenticationType::MicrosoftEntraId;
            displayCatalogAuthInfo.MicrosoftEntraIdInfo = std::move(displayCatalogMicrosoftEntraIdAuthInfo);

            m_displayCatalogAuthenticator = std::make_unique<Authentication::Authenticator>(std::move(displayCatalogAuthInfo), authArgs);
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        if (!TestHooks::s_Licensing_HttpPipelineStage_Override)
#endif
        {
            Authentication::MicrosoftEntraIdAuthenticationInfo licensingMicrosoftEntraIdAuthInfo;
            licensingMicrosoftEntraIdAuthInfo.Resource = "c5e1cb0d-5d24-4b1a-b291-ec684152b2ba";
            Authentication::AuthenticationInfo licensingAuthInfo;
            licensingAuthInfo.Type = Authentication::AuthenticationType::MicrosoftEntraId;
            licensingAuthInfo.MicrosoftEntraIdInfo = std::move(licensingMicrosoftEntraIdAuthInfo);

            m_licensingAuthenticator = std::make_unique<Authentication::Authenticator>(std::move(licensingAuthInfo), authArgs);
        }
    }

    MSStoreDownloadInfo MSStoreDownloadContext::GetDownloadInfo()
    {
#ifndef WINGET_DISABLE_FOR_FUZZING
        auto displayCatalogPackage = DisplayCatalogDetails::CallDisplayCatalogAndGetPreferredPackage(m_productId, m_locale, m_architecture, GetAuthHeaders(m_displayCatalogAuthenticator));
        auto downloadInfo = SfsClientDetails::CallSfsClientAndGetMSStoreDownloadInfo(displayCatalogPackage.WuCategoryId, m_architecture, m_platform);
        downloadInfo.ContentId = displayCatalogPackage.ContentId;
        return downloadInfo;
#else
        return {};
#endif
    }

    std::vector<BYTE> MSStoreDownloadContext::GetLicense(std::string_view contentId)
    {
        return LicensingDetails::GetLicensing(contentId, GetAuthHeaders(m_licensingAuthenticator));
    }
}
