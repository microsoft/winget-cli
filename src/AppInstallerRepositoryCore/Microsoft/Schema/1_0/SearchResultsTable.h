// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteTempTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    // Table for holding temporary search results.
    struct SearchResultsTable : public TempTable
    {
        SearchResultsTable(Connection& connection);

        SearchResultsTable(const SearchResultsTable&) = delete;
        SearchResultsTable& operator=(const SearchResultsTable&) = delete;

        SearchResultsTable(SearchResultsTable&&) = default;
        SearchResultsTable& operator=(SearchResultsTable&&) = default;


    };
}
