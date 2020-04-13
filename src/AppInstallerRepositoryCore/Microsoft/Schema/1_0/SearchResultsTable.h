// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteTempTable.h"
#include "Microsoft/Schema/ISQLiteIndex.h"
#include "AppInstallerRepositorySearch.h"

#include <utility>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Table for holding temporary search results.
    struct SearchResultsTable : public SQLite::TempTable
    {
        SearchResultsTable(SQLite::Connection& connection);

        SearchResultsTable(const SearchResultsTable&) = delete;
        SearchResultsTable& operator=(const SearchResultsTable&) = delete;

        SearchResultsTable(SearchResultsTable&&) = default;
        SearchResultsTable& operator=(SearchResultsTable&&) = default;

        // Performs the requested search type on the requested field.
        void SearchOnField(ApplicationMatchField field, MatchType match, std::string_view value);

        // Removes rows with manifest ids whose sort order is below the highest one.
        void RemoveDuplicateManifestRows();

        // Prepares the table for a filtering pass.
        void PrepareToFilter();

        // Performs the requested filter type on the requested field.
        void FilterOnField(ApplicationMatchField field, MatchType match, std::string_view value);

        // Completes a filtering pass, removing filtered rows.
        void CompleteFilter();

        // Gets the results from the table.
        ISQLiteIndex::SearchResult GetSearchResults(size_t limit = 0);

    private:
        SQLite::Connection& m_connection;
        int m_sortOrdinalValue = 0;
    };
}
