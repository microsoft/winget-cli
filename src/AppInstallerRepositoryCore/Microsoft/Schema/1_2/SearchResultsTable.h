// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/1_1/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
{
    // Table for holding temporary search results.
    struct SearchResultsTable : public V1_1::SearchResultsTable
    {
        SearchResultsTable(const SQLite::Connection& connection) : V1_1::SearchResultsTable(connection) {}

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

        // Import all overrides of this function
        using V1_0::SearchResultsTable::BindStatementForMatchType;

        void BindStatementForMatchType(SQLite::Statement& statement, const PackageMatchFilter& filter, const std::vector<int>& bindIndex) override;
    };
}
