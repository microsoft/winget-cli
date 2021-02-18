// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteTempTable.h"
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "AppInstallerRepositorySearch.h"

#include <optional>
#include <utility>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Table for holding temporary search results.
    struct SearchResultsTable : public SQLite::TempTable
    {
        SearchResultsTable(const SQLite::Connection& connection);

        SearchResultsTable(const SearchResultsTable&) = delete;
        SearchResultsTable& operator=(const SearchResultsTable&) = delete;

        SearchResultsTable(SearchResultsTable&&) = default;
        SearchResultsTable& operator=(SearchResultsTable&&) = default;

        // Performs the requested search type on the requested field.
        void SearchOnField(const PackageMatchFilter& filter);

        // Removes rows with manifest ids whose sort order is below the highest one.
        void RemoveDuplicateManifestRows();

        // Prepares the table for a filtering pass.
        void PrepareToFilter();

        // Performs the requested filter type on the requested field.
        void FilterOnField(const PackageMatchFilter& filter);

        // Completes a filtering pass, removing filtered rows.
        void CompleteFilter();

        // Gets the results from the table.
        ISQLiteIndex::SearchResult GetSearchResults(size_t limit = 0);

    protected:
        // Builds the search statement for the specified field and match type.
        std::vector<int> BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, PackageMatchField field, MatchType match) const;

        virtual std::vector<int> BuildSearchStatement(
            SQLite::Builder::StatementBuilder& builder,
            PackageMatchField field,
            std::string_view manifestAlias,
            std::string_view valueAlias,
            bool useLike) const;

        static bool MatchUsesLike(MatchType match);
        void BindStatementForMatchType(SQLite::Statement& statement, MatchType match, int bindIndex, std::string_view value);

        virtual void BindStatementForMatchType(SQLite::Statement& statement, const PackageMatchFilter& filter, const std::vector<int>& bindIndex);

    private:
        const SQLite::Connection& m_connection;
        int m_sortOrdinalValue = 0;
    };
}
