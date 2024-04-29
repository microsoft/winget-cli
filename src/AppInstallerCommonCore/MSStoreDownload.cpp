// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <AppinstallerLogging.h>
#include "AppInstallerRuntime.h"
#include "winget/Locale.h"
#include "winget/MSStoreDownload.h"
#include "winget/Rest.h"
#include "winget/HttpClientHelper.h"
#include "winget/UserSettings.h"

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;

    namespace DisplayCatalogDetails
    {
        // Default preferred sku to use
        constexpr std::string_view TargetSkuIdValue = "0010"sv;

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
        constexpr std::string_view MSStoreCatalogRestApi = R"(https://displaycatalog.mp.microsoft.com/v7.0/products/{0}?fieldsTemplate={1}&market={2}&languages={3})";
        constexpr std::string_view Details = "Details"sv;
        constexpr std::string_view Neutral = "Neutral"sv;


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

        // Display catalog package comparison logic

        namespace DisplayCatalogPackageComparison
        {
            struct DiaplayCatalogPackageComparisonField
            {
                DiaplayCatalogPackageComparisonField(std::string_view name) : m_name(name) {}

                virtual ~DiaplayCatalogPackageComparisonField() = default;

                std::string_view Name() const { return m_name; }

                virtual bool IsApplicable(const DisplayCatalogPackage& package) = 0;

                virtual bool IsFirstBetter(const DisplayCatalogPackage& first, const DisplayCatalogPackage& second) = 0;

            private:
                std::string_view m_name;
            };

            struct PackageFormatComparator : public DiaplayCatalogPackageComparisonField
            {
                PackageFormatComparator() : DiaplayCatalogPackageComparisonField("Package Format") {}

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

            struct LocaleComparator : public DiaplayCatalogPackageComparisonField
            {
                LocaleComparator(std::string locale) : DiaplayCatalogPackageComparisonField("Locale")
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

            struct ArchitectureComparator : public DiaplayCatalogPackageComparisonField
            {
                ArchitectureComparator(Utility::Architecture architecture) : DiaplayCatalogPackageComparisonField("Architecture")
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
                void AddComparator(std::unique_ptr<DiaplayCatalogPackageComparisonField>&& comparator)
                {
                    if (comparator)
                    {
                        m_comparators.emplace_back(std::move(comparator));
                    }
                }

                std::vector<std::unique_ptr<DiaplayCatalogPackageComparisonField>> m_comparators;
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

            auto restEndpoint = AppInstaller::Utility::Format(std::string{ MSStoreCatalogRestApi },
                productId, Details, AppInstaller::Runtime::GetOSRegion(), Utility::Join(Utility::LocIndView(","), locales));

            return JSON::GetUtilityString(restEndpoint);
        }

        std::reference_wrapper<const web::json::value> GetSkuNodeFromDisplayCatalogResponse(const web::json::value& responseObject)
        {
            AICLI_LOG(Core, Info, << "Started parsing diaplay catalog response. Try to find target sku: " << TargetSkuIdValue);

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

        std::vector<DisplayCatalogPackage> GetDisplayCatalogPackagesFromSkuNode(const web::json::value& jsonObject)
        {
            AICLI_LOG(Core, Info, << "Started extracing diaplay catalog packages from sku.");

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
                std::optional<std::reference_wrapper<const web::json::value>> fulfillmentData = JSON::GetJsonValueFromNode(propertiesValue, JSON::GetUtilityString(FulfillmentData));
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

        DisplayCatalogPackage CallDisplayCatalogAndGetPreferredPackage(std::string_view productId, std::string_view locale, Utility::Architecture architecture)
        {
            auto displayCatalogApi = GetDisplayCatalogRestApi(productId, locale);

            AppInstaller::Http::HttpClientHelper httpClientHelper;
            std::optional<web::json::value> displayCatalogResponseObject = httpClientHelper.HandleGet(displayCatalogApi);

            if (!displayCatalogResponseObject)
            {
                AICLI_LOG(Core, Error, << "No display catalog json object found");
                THROW_HR(APPINSTALLER_CLI_ERROR_DISPLAYCATALOG_API_FAILED);
            }

            const auto& sku = GetSkuNodeFromDisplayCatalogResponse(displayCatalogResponseObject.value());
            auto displayCatalogPackages = GetDisplayCatalogPackagesFromSkuNode(sku.get());

            DisplayCatalogPackageComparison::DisplayCatalogPackageComparator packageComparator(std::string{ locale }, architecture);
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



}
