// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <AppinstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <winget/MSStoreRest.h>
#include <winget/Rest.h>

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;

    namespace
    {
        constexpr std::string_view Neutral = "Neutral"sv;
        constexpr std::string_view DefaultPreferredSkuIdValue = "0010"sv;
        constexpr std::string_view DisplaySkuAvailabilities = "DisplaySkuAvailabilities"sv;
        constexpr std::string_view FulfillmentData = "FulfillmentData"sv;
        constexpr std::string_view PreferredSkuId = "PreferredSkuId"sv;
        constexpr std::string_view Product = "Product"sv;
        constexpr std::string_view Properties = "Properties"sv;
        constexpr std::string_view Sku = "Sku"sv;
        constexpr std::string_view SkuId = "SkuId"sv;
        constexpr std::string_view WuCategoryId = "WuCategoryId"sv;
        constexpr std::string_view MSStoreCatalogRestApi = R"(https://displaycatalog.mp.microsoft.com/v7.0/products/{0}?fieldsTemplate=InstallAgent&market={1}&languages={2})";

        std::optional<std::string> GetWuCategoryIdInternal(const web::json::value& jsonObject)
        {
            try
            {
                if (jsonObject.is_null())
                {
                    AICLI_LOG(Repo, Error, << "Missing json object.");
                    return {};
                }

                std::optional<std::reference_wrapper<const web::json::value>> product = JSON::GetJsonValueFromNode(jsonObject, JSON::GetUtilityString(Product));
                if (!product)
                {
                    AICLI_LOG(Repo, Error, << "Missing Product");
                    return {};
                }

                const auto& productValue = product.value().get();

                // Retrieve the preferred sku id. If it does not exist, default to '0010'.
                std::optional<std::string> preferredSkuId = JSON::GetRawStringValueFromJsonNode(productValue, JSON::GetUtilityString(PreferredSkuId));
                std::string preferredSkuIdValue = JSON::IsValidNonEmptyStringValue(preferredSkuId) ? preferredSkuId.value() : std::string{ DefaultPreferredSkuIdValue };
                AICLI_LOG(Repo, Info, << "Preferred sku id: " << preferredSkuIdValue);

                // Find sku with the preferred sku id.
                std::optional<std::reference_wrapper<const web::json::value>> preferredSku;

                auto skuEntries = JSON::GetRawJsonArrayFromJsonNode(productValue, JSON::GetUtilityString(DisplaySkuAvailabilities));
                if (!skuEntries)
                {
                    AICLI_LOG(Repo, Error, << "Missing DisplaySkuAvailabilities");
                    return {};
                }

                for (const auto& skuEntry : skuEntries.value().get())
                {
                    std::optional<std::reference_wrapper<const web::json::value>> sku = JSON::GetJsonValueFromNode(skuEntry, JSON::GetUtilityString(Sku));
                    if (!sku)
                    {
                        AICLI_LOG(Repo, Error, << "Missing Sku");
                        return {};
                    }

                    const auto& skuValue = sku.value().get();
                    std::optional<std::string> skuId = JSON::GetRawStringValueFromJsonNode(skuValue, JSON::GetUtilityString(SkuId));
                    if (JSON::IsValidNonEmptyStringValue(skuId) && Utility::CaseInsensitiveEquals(skuId.value(), preferredSkuIdValue))
                    {
                        preferredSku = sku;
                        break;
                    }
                }

                // If there is no sku that matches the preferred id, default to using the first sku entry.
                if (!preferredSku)
                {
                    preferredSku = skuEntries.value().get().at(0);
                    AICLI_LOG(Repo, Info, << "Sku entry with preferred id not found. Using the first sku entry.");
                }

                const auto& preferredSkuValue = preferredSku.value().get();
                std::optional<std::reference_wrapper<const web::json::value>> properties = JSON::GetJsonValueFromNode(preferredSkuValue, JSON::GetUtilityString(Properties));
                if (!properties)
                {
                    AICLI_LOG(Repo, Error, << "Missing Properties");
                    return {};
                }

                const auto& propertiesValue = properties.value().get();
                std::optional<std::reference_wrapper<const web::json::value>> fulfillmentData = JSON::GetJsonValueFromNode(propertiesValue, JSON::GetUtilityString(FulfillmentData));
                if (!fulfillmentData)
                {
                    AICLI_LOG(Repo, Error, << "Missing FulfillmentData");
                    return {};
                }

                const auto& fulfillmentDataValue = fulfillmentData.value().get();
                std::optional<std::string> wuCategoryId = JSON::GetRawStringValueFromJsonNode(fulfillmentDataValue, JSON::GetUtilityString(WuCategoryId));
                if (JSON::IsValidNonEmptyStringValue(wuCategoryId))
                {
                    return wuCategoryId.value();
                }
                else
                {
                    return {};
                }
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(Repo, Error, << "Error encountered while deserializing MSStore request. Reason: " << e.what());
            }
            catch (...)
            {
                AICLI_LOG(Repo, Error, << "Received invalid information.");
            }
        }
    }

    std::string GetWuCategoryId(const web::json::value& jsonObject)
    {
        std::optional<std::string> wuCategoryId = GetWuCategoryIdInternal(jsonObject);

        THROW_HR_IF(E_UNEXPECTED, !wuCategoryId);

        return wuCategoryId.value();
    }

    std::string GetStoreCatalogRestApi(const std::string& productId, const std::string& locale)
    {
        std::string languageValue = !locale.empty() ? locale : std::string{ Neutral };
        std::string market = AppInstaller::Runtime::GetOSRegion();

        auto restEndpoint = AppInstaller::Utility::Format(std::string{ MSStoreCatalogRestApi }, productId, market, languageValue);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !AppInstaller::Rest::IsValidUri(JSON::GetUtilityString(restEndpoint)));
        return restEndpoint;
    }
}
