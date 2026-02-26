// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/JsonUtil.h>
#include "Rest/Schema/IRestClient.h"

#include <memory>
#include <vector>

namespace AppInstaller::Repository::Rest::Schema
{
    // Exposes functions for parsing JSON REST responses to IRestClient SearchResult.
    struct SearchResponseParser
    {
        SearchResponseParser(const Utility::Version& schemaVersion);

        SearchResponseParser(const SearchResponseParser&) = delete;
        SearchResponseParser& operator=(const SearchResponseParser&) = delete;

        SearchResponseParser(SearchResponseParser&&) noexcept;
        SearchResponseParser& operator=(SearchResponseParser&&) noexcept;

        ~SearchResponseParser();

        // Gets the search result for response object
        IRestClient::SearchResult Deserialize(const web::json::value& searchResultJsonObject) const;

    private:
        struct impl;
        std::unique_ptr<impl> m_pImpl;
    };
}
