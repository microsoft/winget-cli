// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <AppinstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <winget/Locale.h>
#include <winget/MSStoreDownload.h>
#include <winget/Rest.h>
#include <winget/UserSettings.h>

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;

    namespace
    {
        constexpr std::string_view Architectures = "Architectures"sv;
        constexpr std::string_view ContentId = "ContentId"sv;
        constexpr std::string_view Neutral = "Neutral"sv;
        constexpr std::string_view DefaultSkuIdValue = "0010"sv;
        constexpr std::string_view Details = "Details"sv;
        constexpr std::string_view DisplaySkuAvailabilities = "DisplaySkuAvailabilities"sv;
        constexpr std::string_view FulfillmentData = "FulfillmentData"sv;
        constexpr std::string_view Languages = "Languages"sv;
        constexpr std::string_view PackageFormat = "PackageFormat"sv;
        constexpr std::string_view PackageId = "PackageId"sv;
        constexpr std::string_view Packages = "Packages"sv;
        constexpr std::string_view PreferredSkuId = "PreferredSkuId"sv;
        constexpr std::string_view Product = "Product"sv;
        constexpr std::string_view Properties = "Properties"sv;
        constexpr std::string_view Sku = "Sku"sv;
        constexpr std::string_view SkuId = "SkuId"sv;
        constexpr std::string_view WuCategoryId = "WuCategoryId"sv;
        constexpr std::string_view MSStoreCatalogRestApi = R"(https://displaycatalog.mp.microsoft.com/v7.0/products/{0}?fieldsTemplate={1}&market={2}&languages={3})";

        bool IsPackageFormatBundle(PackageFormatEnum packageFormatEnum)
        {
            return
                packageFormatEnum == PackageFormatEnum::AppxBundle ||
                packageFormatEnum == PackageFormatEnum::MsixBundle;
        }

        struct PackageFormatComparator : public details::MSStoreCatalogPackageComparisonField
        {
            PackageFormatComparator() : details::MSStoreCatalogPackageComparisonField("Package Format") {}

            bool IsApplicable(const MSStoreCatalogPackage& package) override
            {
                if (package.PackageFormat == PackageFormatEnum::EAppxBundle)
                {
                    return false;
                }

                return true;
            }

            bool IsFirstBetter(const MSStoreCatalogPackage& first, const MSStoreCatalogPackage& second) override
            {
                return IsPackageFormatBundle(first.PackageFormat) && !IsPackageFormatBundle(second.PackageFormat);
            }
        };

        struct LanguageComparator : public details::MSStoreCatalogPackageComparisonField
        {
            LanguageComparator(std::vector<std::string> preference, std::vector<std::string> requirement) :
                details::MSStoreCatalogPackageComparisonField("Language"), m_preference(std::move(preference)), m_requirement(std::move(requirement))
            {
                AICLI_LOG(Core, Verbose,
                    << "Language Comparator created with Required Languages: " << Utility::ConvertContainerToString(m_requirement)
                    << " , Preferred Languages: " << Utility::ConvertContainerToString(m_preference));
            }

            static std::unique_ptr<LanguageComparator> Create(const std::vector<std::string>& requiredLanguages)
            {
                std::vector<std::string> requirement = requiredLanguages;
                if (requirement.empty())
                {
                    requirement = Settings::User().Get<Settings::Setting::InstallLocaleRequirement>();
                }

                std::vector<std::string> preference = Settings::User().Get<Settings::Setting::InstallLocalePreference>();
                if (preference.empty())
                {
                    preference = AppInstaller::Locale::GetUserPreferredLanguages();
                }

                if (!preference.empty() || !requirement.empty())
                {
                    return std::make_unique<LanguageComparator>(preference, requirement);
                }
                else
                {
                    return {};
                }
            }

            bool IsApplicable(const MSStoreCatalogPackage& package) override
            {
                if (!m_requirement.empty())
                {
                    for (auto const& requiredLanguage : m_requirement)
                    {
                        double distanceScore = GetBestDistanceScoreFromList(requiredLanguage, package.Languages);
                        if (distanceScore >= Locale::MinimumDistanceScoreAsPerfectMatch)
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

            bool IsFirstBetter(const MSStoreCatalogPackage& first, const MSStoreCatalogPackage& second)
            {
                if (m_preference.empty())
                {
                    return false;
                }

                for (auto const& preferredLanguage : m_preference)
                {
                    double firstScore = GetBestDistanceScoreFromList(preferredLanguage, first.Languages);
                    double secondScore = GetBestDistanceScoreFromList(preferredLanguage, second.Languages);

                    if (firstScore >= Locale::MinimumDistanceScoreAsCompatibleMatch || secondScore >= Locale::MinimumDistanceScoreAsCompatibleMatch)
                    {
                        return firstScore > secondScore;
                    }
                }

                return false;
            }

        private:
            double GetBestDistanceScoreFromList(std::string_view targetLanguage, const std::vector<std::string>& languages)
            {
                double finalScore{};
                for (auto const& language : languages)
                {
                    double currentScore = Locale::GetDistanceOfLanguage(targetLanguage, language);
                    if (currentScore > finalScore)
                    {
                        finalScore = currentScore;
                    }
                }

                return finalScore;
            }

            std::vector<std::string> m_preference;
            std::vector<std::string> m_requirement;
        };

        struct ArchitectureComparator : public details::MSStoreCatalogPackageComparisonField
        {
            ArchitectureComparator(std::vector<Utility::Architecture> requirement, std::vector<Utility::Architecture> preference) :
                details::MSStoreCatalogPackageComparisonField("Architecture"), m_requirement(std::move(requirement)), m_preference(std::move(preference))
            {
                AICLI_LOG(Core, Verbose,
                    << "Architecture Comparator created with required archs: " << Utility::ConvertContainerToString(m_requirement, Utility::ToString)
                    << " , Preferred archs: " << Utility::ConvertContainerToString(m_preference, Utility::ToString));
            }

            static std::unique_ptr<ArchitectureComparator> Create(const std::vector<Utility::Architecture>& allowedArchitectures)
            {
                std::vector<Utility::Architecture> requiredArchitectures = allowedArchitectures;

                if (requiredArchitectures.empty())
                {
                    requiredArchitectures = Settings::User().Get<Settings::Setting::InstallArchitectureRequirement>();
                }

                std::vector<Utility::Architecture> optionalArchitectures = Settings::User().Get<Settings::Setting::InstallArchitecturePreference>();

                if (!requiredArchitectures.empty() || !optionalArchitectures.empty())
                {
                    return std::make_unique<ArchitectureComparator>(std::move(requiredArchitectures), std::move(optionalArchitectures));
                }
                else
                {
                    return {};
                }
            }

            bool IsApplicable(const MSStoreCatalogPackage& package) override
            {
                if (!m_requirement.empty())
                {
                    return ContainCommonArchitectures(m_requirement, package.Architectures);
                }

                return true;
            }

            bool IsFirstBetter(const MSStoreCatalogPackage& first, const MSStoreCatalogPackage& second) override
            {
                if (!m_preference.empty())
                {
                    return (ContainCommonArchitectures(first.Architectures, m_preference) && !ContainCommonArchitectures(second.Architectures, m_preference));
                }

                return false;
            }

        private:
            // Checks if two lists of architectures have common elements.
            bool ContainCommonArchitectures(const std::vector<Utility::Architecture>& firstList, const std::vector<Utility::Architecture>& secondList)
            {
                for (auto arch : firstList)
                {
                    if (IsArchitectureInList(arch, secondList))
                    {
                        return true;
                    }
                }

                return false;
            }

            bool IsArchitectureInList(Utility::Architecture arch, const std::vector<Utility::Architecture>& architectureList)
            {
                return architectureList.end() != std::find_if(
                    architectureList.begin(),
                    architectureList.end(),
                    [&](const auto& a) { return a == arch; });
            }

            std::vector<Utility::Architecture> m_requirement;
            std::vector<Utility::Architecture> m_preference;
        };
    }

    DisplayCatalogPackageComparator::DisplayCatalogPackageComparator(const std::vector<std::string>& requiredLanguages, const std::vector<Utility::Architecture>& requiredArchs)
    {
        // Order of comparators matters.
        AddComparator(LanguageComparator::Create(requiredLanguages));
        AddComparator(ArchitectureComparator::Create(requiredArchs));
        AddComparator(std::make_unique<PackageFormatComparator>());
    }

    void DisplayCatalogPackageComparator::AddComparator(std::unique_ptr<details::MSStoreCatalogPackageComparisonField>&& comparator)
    {
        if (comparator)
        {
            m_comparators.emplace_back(std::move(comparator));
        }
    }

    std::optional<MSStoreCatalogPackage> DisplayCatalogPackageComparator::GetPreferredPackage(const std::vector<MSStoreCatalogPackage>& packages)
    {
        AICLI_LOG(Core, Verbose, << "Starting MSStore package selection.");

        const MSStoreCatalogPackage* result = nullptr;
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

    bool DisplayCatalogPackageComparator::IsFirstBetter(const MSStoreCatalogPackage& first, const MSStoreCatalogPackage& second)
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

    bool DisplayCatalogPackageComparator::IsApplicable(const MSStoreCatalogPackage& package)
    {
        for (const auto& comparator : m_comparators)
        {
            bool result = comparator->IsApplicable(package);
            if (!result)
            {
                return false;
            }
        }

        return true;
    }

    PackageFormatEnum ConvertToPackageFormatEnum(std::string_view packageFormatStr)
    {
        std::string packageFormat = Utility::ToLower(packageFormatStr);
        if (packageFormat == "appxbundle")
        {
            return PackageFormatEnum::AppxBundle;
        }
        else if (packageFormat == "eappxbundle")
        {
            return PackageFormatEnum::EAppxBundle;
        }
        else if (packageFormat == "msixbundle")
        {
            return PackageFormatEnum::MsixBundle;
        }
        else if (packageFormat == "appx")
        {
            return PackageFormatEnum::Appx;
        }
        else if (packageFormat == "msix")
        {
            return PackageFormatEnum::Msix;
        }

        AICLI_LOG(Core, Info, << "ConvertToPackageFormatEnum: Unknown package format: " << packageFormatStr);
        return PackageFormatEnum::Unknown;
    }

    std::vector<MSStoreCatalogPackage> DeserializeMSStoreCatalogPackages(const web::json::value& jsonObject)
    {
        try
        {
            if (jsonObject.is_null())
            {
                AICLI_LOG(Core, Error, << "Missing json object.");
                return {};
            }

            std::optional<std::reference_wrapper<const web::json::value>> product = JSON::GetJsonValueFromNode(jsonObject, JSON::GetUtilityString(Product));
            if (!product)
            {
                AICLI_LOG(Core, Error, << "Missing Product");
                return {};
            }

            const auto& productValue = product.value().get();

            std::optional<std::reference_wrapper<const web::json::value>> defaultSku;

            auto skuEntries = JSON::GetRawJsonArrayFromJsonNode(productValue, JSON::GetUtilityString(DisplaySkuAvailabilities));
            if (!skuEntries)
            {
                AICLI_LOG(Core, Error, << "Missing DisplaySkuAvailabilities");
                return {};
            }

            for (const auto& skuEntry : skuEntries.value().get())
            {
                std::optional<std::reference_wrapper<const web::json::value>> sku = JSON::GetJsonValueFromNode(skuEntry, JSON::GetUtilityString(Sku));
                if (!sku)
                {
                    AICLI_LOG(Core, Error, << "Missing Sku");
                    return {};
                }

                const auto& skuValue = sku.value().get();
                std::optional<std::string> skuId = JSON::GetRawStringValueFromJsonNode(skuValue, JSON::GetUtilityString(SkuId));
                if (JSON::IsValidNonEmptyStringValue(skuId) && Utility::CaseInsensitiveEquals(skuId.value(), DefaultSkuIdValue))
                {
                    defaultSku = sku;
                    break;
                }
            }

            if (!defaultSku)
            {
                AICLI_LOG(Core, Error, << "Default Sku (" << DefaultSkuIdValue << ") not found");
                return {};
            }

            const auto& defaultSkuValue = defaultSku.value().get();
            std::optional<std::reference_wrapper<const web::json::value>> properties = JSON::GetJsonValueFromNode(defaultSkuValue, JSON::GetUtilityString(Properties));
            if (!properties)
            {
                AICLI_LOG(Core, Error, << "Missing Properties");
                return {};
            }

            const auto& propertiesValue = properties.value().get();
            auto packages = JSON::GetRawJsonArrayFromJsonNode(propertiesValue, JSON::GetUtilityString(Packages));
            if (!packages)
            {
                AICLI_LOG(Core, Error, << "Missing Packages");
                return {};
            }

            std::vector<MSStoreCatalogPackage> displayCatalogPackages;

            for (const auto& packageEntry : packages.value().get())
            {
                MSStoreCatalogPackage catalogPackage;

                // Package Id
                std::optional<std::string> packageId = JSON::GetRawStringValueFromJsonNode(packageEntry, JSON::GetUtilityString(PackageId));
                if (packageId)
                {
                    catalogPackage.PackageId = packageId.value();
                }

                // Architectures
                auto architectures = JSON::GetRawStringArrayFromJsonNode(packageEntry, JSON::GetUtilityString(Architectures));
                for (const auto& arch : architectures)
                {
                    catalogPackage.Architectures.emplace_back(Utility::ConvertToArchitectureEnum(arch));
                }

                // Languages
                auto languages = JSON::GetRawStringArrayFromJsonNode(packageEntry, JSON::GetUtilityString(Languages));
                for (const auto& language : languages)
                {
                    catalogPackage.Languages.emplace_back(language);
                }

                // Package Format
                std::optional<std::string> packageFormat = JSON::GetRawStringValueFromJsonNode(packageEntry, JSON::GetUtilityString(PackageFormat));
                if (packageFormat)
                {
                    catalogPackage.PackageFormat = ConvertToPackageFormatEnum(packageFormat.value());
                }

                // Content Id
                std::optional<std::string> contentId = JSON::GetRawStringValueFromJsonNode(packageEntry, JSON::GetUtilityString(ContentId));
                if (contentId)
                {
                    catalogPackage.ContentId = contentId.value();
                }

                // WuCategoryId
                std::optional<std::reference_wrapper<const web::json::value>> fulfillmentData = JSON::GetJsonValueFromNode(propertiesValue, JSON::GetUtilityString(FulfillmentData));
                if (fulfillmentData)
                {
                    const auto& fulfillmentDataValue = fulfillmentData.value().get();
                    std::optional<std::string> wuCategoryId = JSON::GetRawStringValueFromJsonNode(fulfillmentDataValue, JSON::GetUtilityString(WuCategoryId));
                    if (JSON::IsValidNonEmptyStringValue(wuCategoryId))
                    {
                        catalogPackage.WuCategoryId = wuCategoryId.value();
                    }
                }

                displayCatalogPackages.emplace_back(catalogPackage);
            }

            return displayCatalogPackages;
        }
        catch (const std::exception& e)
        {
            AICLI_LOG(Core, Error, << "Error encountered while deserializing MSStore request. Reason: " << e.what());
        }
        catch (...)
        {
            AICLI_LOG(Core, Error, << "Received invalid information.");
        }
    }

    std::string GetMSStoreCatalogRestApi(const std::string& productId, const std::string& locale)
    {
        std::string languageValue = !locale.empty() ? locale : std::string{ Neutral };
        std::string market = AppInstaller::Runtime::GetOSRegion();

        auto restEndpoint = AppInstaller::Utility::Format(std::string{ MSStoreCatalogRestApi }, productId, Details, market, languageValue);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !AppInstaller::Rest::IsValidUri(JSON::GetUtilityString(restEndpoint)));
        return restEndpoint;
    }

}
