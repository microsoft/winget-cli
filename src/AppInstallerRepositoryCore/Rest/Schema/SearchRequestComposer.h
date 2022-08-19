// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <winget/JsonUtil.h>
#include <winget/RepositorySearch.h>

#include <memory>
#include <vector>

namespace AppInstaller::Repository::Rest::Schema
{
    // Exposes functions for creating JSON REST request body from search requests.
    struct SearchRequestComposer
    {
        SearchRequestComposer(const Utility::Version& schemaVersion);

        SearchRequestComposer(const SearchRequestComposer&) = delete;
        SearchRequestComposer& operator=(const SearchRequestComposer&) = delete;

        SearchRequestComposer(SearchRequestComposer&&) noexcept;
        SearchRequestComposer& operator=(SearchRequestComposer&&) noexcept;

        ~SearchRequestComposer();

        // Create search request rest call body from a given SearchRequest
        web::json::value Serialize(const AppInstaller::Repository::SearchRequest& searchRequest) const;

    private:
        struct impl;
        std::unique_ptr<impl> m_pImpl;
    };
}
