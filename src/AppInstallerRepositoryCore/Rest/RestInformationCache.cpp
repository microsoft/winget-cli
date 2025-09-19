// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestInformationCache.h"
#include "Rest/Schema/InformationResponseDeserializer.h"
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest
{
    namespace
    {
        constexpr std::wstring_view s_EndpointName = L"endpoint"sv;
        constexpr std::wstring_view s_HashName = L"hash"sv;
        constexpr std::wstring_view s_ExpirationName = L"expiration"sv;
        constexpr std::wstring_view s_DataName = L"data"sv;

        // Calculates the hash of values that might change per-call.
        Utility::SHA256::HashBuffer GetHash(const std::optional<std::string>& customHeader, std::string_view caller)
        {
            std::stringstream stream;
            if (customHeader)
            {
                stream << customHeader.value();
            }
            stream << '|' << caller;

            return Utility::SHA256::ComputeHash(stream);
        }

        uint64_t CalculateExpiration(std::chrono::seconds duration)
        {
            // If no expiration information is provided, use 1 minute
            if (!duration.count())
            {
                duration = 60s;
            }

            return Utility::ConvertSystemClockToUnixEpoch(std::chrono::system_clock::now() + duration);
        }
    }

    std::optional<Schema::IRestClient::Information> RestInformationCache::Get(const std::wstring& endpoint, const std::optional<std::string>& customHeader, std::string_view caller)
#ifdef AICLI_DISABLE_TEST_HOOKS
        try
#endif
    {
        LoadCacheView();

        Utility::SHA256::HashBuffer hashValue = GetHash(customHeader, caller);
        CacheItem* item = FindCacheItem(endpoint, hashValue);

        // If we don't find a private match, see if there is a public one.
        if (!item)
        {
            item = FindCacheItem(endpoint, {});
        }

        if (!item)
        {
            return std::nullopt;
        }

        Schema::InformationResponseDeserializer responseDeserializer;
        return responseDeserializer.Deserialize(item->Data);
    }
#ifdef AICLI_DISABLE_TEST_HOOKS
    catch (...)
    {
        LOG_CAUGHT_EXCEPTION_MSG("RestInformationCache::Get exception");
        return std::nullopt;
    }
#endif

    void RestInformationCache::Cache(const std::wstring& endpoint, const std::optional<std::string>& customHeader, std::string_view caller, const Utility::CacheControlPolicy& cacheControl, web::json::value response)
#ifdef AICLI_DISABLE_TEST_HOOKS
        try
#endif
    {
        // If requested, do not cache this response.
        // Since this data is small, treat no-cache as no-store.
        if (cacheControl.NoStore || cacheControl.NoCache)
        {
            return;
        }

        // If not public, we use the header values to differentiate the cache items.
        Utility::SHA256::HashBuffer hashValue;
        if (!cacheControl.Public)
        {
            hashValue = GetHash(customHeader, caller);
        }

        uint64_t expirationEpoch = CalculateExpiration(std::chrono::seconds{ cacheControl.MaxAge });

        // Due to the exchange semantics on the setting stream, we may have to retry storing the value.
        for (int i = 0; i < 10; ++i)
        {
            CacheItem* item = FindCacheItem(endpoint, hashValue);

            if (!item)
            {
                item = &m_cacheView.emplace_back();

                item->Endpoint = endpoint;
                item->Hash = hashValue;
            }

            item->UnixEpochExpiration = expirationEpoch;
            item->Data = std::move(response);

            if (StoreCacheView())
            {
                AICLI_LOG(Repo, Verbose, << "RestInformationCache stored information for: " << Utility::ConvertToUTF8(endpoint));
                return;
            }
            else
            {
                // Extract the response back from the item for the next iteration
                response = std::move(item->Data);

                // Failed to store due to the cache changing, reload and try again.
                LoadCacheView();
            }
        }

        AICLI_LOG(Repo, Warning, << "RestInformationCache failed to store information cache after 10 attempts.");
    }
#ifdef AICLI_DISABLE_TEST_HOOKS
    CATCH_LOG();
#endif

    void RestInformationCache::LoadCacheView()
    {
        using namespace web::json;

        std::unique_ptr<std::istream> stream = m_settingsStream.Get();
        m_cacheView.clear();

        if (!stream)
        {
            return;
        }

        value cacheValue = value::parse(*stream);

        if (!cacheValue.is_array())
        {
            AICLI_LOG(Repo, Warning, << "RestInformationCache value was not an array.");
            return;
        }

        array& cacheArray = cacheValue.as_array();

        for (const value& cacheItemValue : cacheArray)
        {
            if (!cacheItemValue.is_object())
            {
                AICLI_LOG(Repo, Warning, << "RestInformationCache cache item was not an object.");
                continue;
            }

            std::optional<uint64_t> expiration = JSON::GetRawUInt64ValueFromJsonNode(cacheItemValue, std::wstring{ s_ExpirationName });
            if (!expiration)
            {
                AICLI_LOG(Repo, Warning, << "RestInformationCache cache item missing expiration.");
                continue;
            }

            if (std::chrono::system_clock::now() > Utility::ConvertUnixEpochToSystemClock(expiration.value()))
            {
                AICLI_LOG(Repo, Verbose, << "RestInformationCache cache item has expired.");
                continue;
            }

            std::optional<std::wstring> endpoint = JSON::GetWideStringValueFromJsonNode(cacheItemValue, std::wstring{ s_EndpointName });
            if (!JSON::IsValidNonEmptyStringValue(endpoint))
            {
                AICLI_LOG(Repo, Warning, << "RestInformationCache cache item missing endpoint.");
                continue;
            }

            CacheItem cacheItem;
            cacheItem.Endpoint = endpoint.value();
            cacheItem.UnixEpochExpiration = expiration.value();

            std::optional<std::wstring> hash = JSON::GetWideStringValueFromJsonNode(cacheItemValue, std::wstring{ s_HashName });
            if (JSON::IsValidNonEmptyStringValue(hash))
            {
                cacheItem.Hash = Utility::SHA256::ConvertToBytes(hash.value());
            }

            auto dataValue = JSON::GetJsonValueFromNode(cacheItemValue, std::wstring{ s_DataName });
            if (!dataValue)
            {
                AICLI_LOG(Repo, Warning, << "RestInformationCache cache item missing data.");
                continue;
            }

            cacheItem.Data = dataValue.value().get();
            if (cacheItem.Data.is_null())
            {
                AICLI_LOG(Repo, Warning, << "RestInformationCache cache item data value null.");
                continue;
            }

            m_cacheView.emplace_back(std::move(cacheItem));
        }
    }

    RestInformationCache::CacheItem* RestInformationCache::FindCacheItem(const std::wstring& endpoint, const Utility::SHA256::HashBuffer& hash)
    {
        for (CacheItem& item : m_cacheView)
        {
            if (item.Endpoint == endpoint &&
                Utility::SHA256::AreEqual(item.Hash, hash))
            {
                return &item;
            }
        }

        return nullptr;
    }

    [[nodiscard]] bool RestInformationCache::StoreCacheView()
    {
        using namespace web::json;

        value cacheValue = value::array();
        array& cacheArray = cacheValue.as_array();

        for (const CacheItem& item : m_cacheView)
        {
            value cacheItemValue = value::object();
            object& cacheItemObject = cacheItemValue.as_object();

            cacheItemObject[std::wstring{ s_EndpointName }] = value::value(item.Endpoint);
            cacheItemObject[std::wstring{ s_HashName }] = value::value(Utility::ConvertToUTF16(Utility::ConvertToHexString(item.Hash)));
            cacheItemObject[std::wstring{ s_ExpirationName }] = value::value(item.UnixEpochExpiration);
            cacheItemObject[std::wstring{ s_DataName }] = item.Data;

            cacheArray[cacheArray.size()] = std::move(cacheItemValue);
        }

        std::stringstream stream;
        cacheValue.serialize(stream);

        return m_settingsStream.Set(std::move(stream).str());
    }
}
