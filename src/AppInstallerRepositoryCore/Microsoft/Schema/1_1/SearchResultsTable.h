// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_0/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    // Table for holding temporary search results.
    struct SearchResultsTable : public V1_0::SearchResultsTable
    {
        SearchResultsTable(const SQLite::Connection& connection) : V1_0::SearchResultsTable(connection) {}

        SearchResultsTable(const SearchResultsTable&) = delete;
        SearchResultsTable& operator=(const SearchResultsTable&) = delete;

        SearchResultsTable(SearchResultsTable&&) = default;
        SearchResultsTable& operator=(SearchResultsTable&&) = default;

    protected:
        std::vector<int> BuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            PackageMatchField field,
            std::string_view manifestAlias,
            std::string_view valueAlias,
            bool useLike) const override;
    };
}
