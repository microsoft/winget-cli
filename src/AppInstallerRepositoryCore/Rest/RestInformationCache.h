// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Rest/Schema/IRestClient.h"
#include <AppInstallerDownloader.h>
#include <AppInstallerSHA256.h>
#include <winget/Settings.h>
#include <cpprest/json.h>
#include <optional>
#include <string>
#include <string_view>

namespace AppInstaller::Repository::Rest
{
    // Provides access to cached responses to the /information request.
    struct RestInformationCache
    {
        // Attempts to get a cached information response for the provided inputs.
        std::optional<Schema::IRestClient::Information> Get(const std::wstring& endpoint, const std::optional<std::string>& customHeader, std::string_view caller);

        // Stores the information response as appropriate.
        void Cache(const std::wstring& endpoint, const std::optional<std::string>& customHeader, std::string_view caller, const Utility::CacheControlPolicy& cacheControl, web::json::value response);

    private:
        struct CacheItem
        {
            std::wstring Endpoint;
            Utility::SHA256::HashBuffer Hash;
            uint64_t UnixEpochExpiration = 0;
            web::json::value Data;
        };

        // Reads from the cache, constructing our view of the items it contains.
        // Discards any expired items while reading the cache.
        void LoadCacheView();

        // Finds the cache item for the given inputs, or nullptr if it is not found.
        CacheItem* FindCacheItem(const std::wstring& endpoint, const Utility::SHA256::HashBuffer& hash);

        // Attempts to store the current cache view back to the cache.
        // Returns true if successful; false if the cache was updated since our last read and a retry is necessary.
        [[nodiscard]] bool StoreCacheView();

        Settings::Stream m_settingsStream{ Settings::Stream::RestInformationCache };
        std::vector<CacheItem> m_cacheView;
    };
}
