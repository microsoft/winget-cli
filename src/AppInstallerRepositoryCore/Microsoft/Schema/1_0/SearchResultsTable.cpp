// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SearchResultsTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_SearchResultsTable_Manifest = "manifest"sv;
    constexpr std::string_view s_SearchResultsTable_MatchField = "field"sv;
    constexpr std::string_view s_SearchResultsTable_MatchType = "match"sv;
    constexpr std::string_view s_SearchResultsTable_MatchRowId = "id"sv;
    constexpr std::string_view s_SearchResultsTable_SortValue = "sort"sv;
    constexpr std::string_view s_SearchResultsTable_Filter = "filter"sv;

    SearchResultsTable::SearchResultsTable(Connection& connection)
    {
        using namespace Builder;

        StatementBuilder builder;
        builder.CreateTable(GetQualifiedName()).BeginColumns();

        builder.Column(ColumnBuilder(s_SearchResultsTable_Manifest, Type::RowId).NotNull());
        builder.Column(ColumnBuilder(s_SearchResultsTable_MatchField, Type::Int).NotNull());
        builder.Column(ColumnBuilder(s_SearchResultsTable_MatchType, Type::Int).NotNull());
        builder.Column(ColumnBuilder(s_SearchResultsTable_MatchRowId, Type::RowId).NotNull());
        builder.Column(ColumnBuilder(s_SearchResultsTable_SortValue, Type::Int).NotNull());
        builder.Column(ColumnBuilder(s_SearchResultsTable_Filter, Type::Bool));

        builder.EndColumns();

        builder.Execute(connection);

        InitDropStatement(connection);
    }
}
