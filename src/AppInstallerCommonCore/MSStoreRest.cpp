// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/MSStoreRest.h>
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <AppinstallerLogging.h>
#include <winget/HttpClientHelper.h>
#include <winget/Rest.h>

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;
    using namespace winrt::Windows::ApplicationModel::Store::Preview::InstallControl;

    namespace
    {
        constexpr std::string_view Product = "Product"sv;
        constexpr std::string_view DisplaySkuAvailabilities = "DisplaySkuAvailabilities"sv;
        constexpr std::string_view Sku = "Sku"sv;
        constexpr std::string_view Properties = "Properties"sv;
        constexpr std::string_view FulfillmentData = "FulfillmentData"sv;
        constexpr std::string_view WuCategoryId = "WuCategoryId"sv;
        static constexpr std::wstring_view MSStoreCatalogApi = L"https://displaycatalog.mp.microsoft.com/v7.0/products/{0}?fieldsTemplate=InstallAgent&market={1}&languages={2}";
    }

    std::string MSStoreRestHelper::GetWuCategoryId(const std::string& productId, const std::string& language, const std::string& marketplace)
    {
        auto restEndpoint = JSON::GetUtilityString(AppInstaller::Utility::Format(Utility::ConvertToUTF8(MSStoreCatalogApi), productId, marketplace, language));
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !AppInstaller::Rest::IsValidUri(restEndpoint));

        AppInstaller::Http::HttpClientHelper httpClientHelper;
        std::optional<web::json::value> jsonObject = httpClientHelper.HandleGet(restEndpoint);

        if (!jsonObject)
        {
            std::cout << "No json object found" << std::endl;
        }

        std::optional<std::string> wuCategoryId = GetWuCategoryIdInternal(jsonObject.value());

        THROW_HR_IF(E_UNEXPECTED, !wuCategoryId);

        return wuCategoryId.value();
    }

    std::optional<std::string> MSStoreRestHelper::GetWuCategoryIdInternal(const web::json::value& jsonObject)
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
            auto skus = JSON::GetRawJsonArrayFromJsonNode(productValue, JSON::GetUtilityString(DisplaySkuAvailabilities));
            if (skus)
            {
                // There are multiple sku in skus, so change this to an array once we are ready.
                const auto& firstEntry = skus.value().get().at(0);

                std::optional<std::reference_wrapper<const web::json::value>> sku = JSON::GetJsonValueFromNode(firstEntry, JSON::GetUtilityString(Sku));
                if (!sku)
                {
                    AICLI_LOG(Repo, Error, << "Missing SKU");
                    return {};
                }

                const auto& skuValue = sku.value().get();
                std::optional<std::reference_wrapper<const web::json::value>> properties = JSON::GetJsonValueFromNode(skuValue, JSON::GetUtilityString(Properties));
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

            return {};
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
