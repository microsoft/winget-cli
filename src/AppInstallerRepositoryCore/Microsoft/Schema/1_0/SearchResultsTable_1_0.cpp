// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SearchResultsTable.h"
#include "SQLiteStatementBuilder.h"

#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/NameTable.h"
#include "Microsoft/Schema/1_0/MonikerTable.h"
#include "Microsoft/Schema/1_0/ManifestTable.h"
#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_0/CommandsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;

        constexpr std::string_view s_SearchResultsTable_Manifest = "manifest"sv;
        constexpr std::string_view s_SearchResultsTable_MatchField = "field"sv;
        constexpr std::string_view s_SearchResultsTable_MatchType = "match"sv;
        constexpr std::string_view s_SearchResultsTable_MatchValue = "value"sv;
        constexpr std::string_view s_SearchResultsTable_SortValue = "sort"sv;
        constexpr std::string_view s_SearchResultsTable_Filter = "filter"sv;

        constexpr std::string_view s_SearchResultsTable_Index_Suffix = "_i_m"sv;

        constexpr std::string_view s_SearchResultsTable_SubSelect_TableAlias = "valueTable"sv;
        constexpr std::string_view s_SearchResultsTable_SubSelect_ManifestAlias = "m"sv;
        constexpr std::string_view s_SearchResultsTable_SubSelect_ValueAlias = "v"sv;
    }

    SearchResultsTable::SearchResultsTable(const SQLite::Connection& connection) :
        m_connection(connection)
    {
        using namespace SQLite::Builder;

        {
            StatementBuilder builder;
            builder.CreateTable(GetQualifiedName()).BeginColumns();

            builder.Column(ColumnBuilder(s_SearchResultsTable_Manifest, Type::RowId).NotNull());
            builder.Column(ColumnBuilder(s_SearchResultsTable_MatchField, Type::Int).NotNull());
            builder.Column(ColumnBuilder(s_SearchResultsTable_MatchType, Type::Int).NotNull());
            builder.Column(ColumnBuilder(s_SearchResultsTable_MatchValue, Type::Text).NotNull());
            builder.Column(ColumnBuilder(s_SearchResultsTable_SortValue, Type::Int).NotNull());
            builder.Column(ColumnBuilder(s_SearchResultsTable_Filter, Type::Bool).NotNull());

            builder.EndColumns();

            builder.Execute(m_connection);
        }

        InitDropStatement(m_connection);

        {
            SQLite::Builder::QualifiedTable index = GetQualifiedName();
            std::string indexName(index.Table);
            indexName += s_SearchResultsTable_Index_Suffix;
            index.Table = indexName;

            StatementBuilder builder;
            builder.CreateIndex(indexName).On(GetQualifiedName().Table).Columns(s_SearchResultsTable_Manifest);

            builder.Execute(m_connection);
        }
    }

    void SearchResultsTable::SearchOnField(const PackageMatchFilter& filter)
    {
        using namespace SQLite::Builder;

        int sortOrdinal = m_sortOrdinalValue++;

        // Create an insert statement to select values into the table as requested.
        // The goal is a statement like this:
        //      INSERT INTO <tempTable>
        //      SELECT valueTable.m, <field>, <match>, valueTable.v, <sort>, <filter> FROM
        //      (SELECT manifest.rowid as m, manifest.id as v from manifest join ids on manifest.id = ids.rowid where ids.id = <value>) AS valueTable
        // Where the subselect is built by the owning table.
        StatementBuilder builder;
        builder.InsertInto(GetQualifiedName()).Select().
            Column(QualifiedColumn(s_SearchResultsTable_SubSelect_TableAlias, s_SearchResultsTable_SubSelect_ManifestAlias)).
            Value(filter.Field).
            Value(filter.Type).
            Column(QualifiedColumn(s_SearchResultsTable_SubSelect_TableAlias, s_SearchResultsTable_SubSelect_ValueAlias)).
            Value(sortOrdinal).
            Value(false).
        From().BeginParenthetical();

        // Add the field specific portion
        std::vector<int> bindIndex = BuildSearchStatement(builder, filter.Field, filter.Type);

        if (bindIndex.empty())
        {
            AICLI_LOG(Repo, Verbose, << "PackageMatchField not supported in this version: " << PackageMatchFieldToString(filter.Field));
            return;
        }

        builder.EndParenthetical().As(s_SearchResultsTable_SubSelect_TableAlias);

        SQLite::Statement statement = builder.Prepare(m_connection);
        BindStatementForMatchType(statement, filter, bindIndex);
        statement.Execute();
        AICLI_LOG(Repo, Verbose, << "Search found " << m_connection.GetChanges() << " rows");
    }

    void SearchResultsTable::RemoveDuplicateManifestRows()
    {
        using namespace SQLite::Builder;

        // Create a delete statement to leave only one row with a given manifest.
        // This will arbitrarily choose one of the rows if multiple have the same lowest sort order.
        // The goal is a statement like this:
        //      DELETE from <temp> where rowid not in (
        //          SELECT rowid from (
        //              SELECT rowid, min(sort) from <temp> group by manifest
        //          )
        //      )
        StatementBuilder builder;
        builder.DeleteFrom(GetQualifiedName()).Where(SQLite::RowIDName).Not().In().BeginParenthetical().
            Select(SQLite::RowIDName).From().BeginParenthetical().
                Select().Column(SQLite::RowIDName).Column(Aggregate::Min, s_SearchResultsTable_SortValue).From(GetQualifiedName()).GroupBy(s_SearchResultsTable_Manifest).
            EndParenthetical().
        EndParenthetical();

        builder.Execute(m_connection);
        AICLI_LOG(Repo, Verbose, << "Removed " << m_connection.GetChanges() << " duplicate rows");
    }

    void SearchResultsTable::PrepareToFilter()
    {
        // Reset all filter values to unselected
        SQLite::Builder::StatementBuilder builder;
        builder.Update(GetQualifiedName()).Set().Column(s_SearchResultsTable_Filter).Equals(false);

        builder.Execute(m_connection);
    }

    void SearchResultsTable::FilterOnField(const PackageMatchFilter& filter)
    {
        using namespace SQLite::Builder;

        // Create an update statement to mark rows that are found by the search.
        // This will arbitrarily choose one of the rows if multiple have the same lowest sort order.
        // The goal is a statement like this:
        //      UPDATE <temp> set filter = 1 where manifest in (
        //          SELECT m from (
        //              SELECT manifest.rowid as m, manifest.id as v from manifest join ids on manifest.id = ids.rowid where ids.id = <value>
        //          )
        //      )
        StatementBuilder builder;
        builder.Update(GetQualifiedName()).Set().Column(s_SearchResultsTable_Filter).Equals(true).Where(s_SearchResultsTable_Manifest).In().BeginParenthetical().
            Select(s_SearchResultsTable_SubSelect_ManifestAlias).From().BeginParenthetical();

        // Add the field specific portion
        std::vector<int> bindIndex = BuildSearchStatement(builder, filter.Field, filter.Type);

        if (bindIndex.empty())
        {
            AICLI_LOG(Repo, Verbose, << "PackageMatchField not supported in this version: " << PackageMatchFieldToString(filter.Field));
            return;
        }

        builder.EndParenthetical().EndParenthetical();

        SQLite::Statement statement = builder.Prepare(m_connection);
        BindStatementForMatchType(statement, filter, bindIndex);
        statement.Execute();
        AICLI_LOG(Repo, Verbose, << "Filter kept " << m_connection.GetChanges() << " rows");
    }

    void SearchResultsTable::CompleteFilter()
    {
        // Delete all unselected values
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(GetQualifiedName()).Where(s_SearchResultsTable_Filter).Equals(false);

        builder.Execute(m_connection);
        AICLI_LOG(Repo, Verbose, << "Filter deleted " << m_connection.GetChanges() << " rows");
    }

    ISQLiteIndex::SearchResult SearchResultsTable::GetSearchResults(size_t limit)
    {
        constexpr std::string_view tempTableAlias = "t"sv;

        using namespace SQLite::Builder;
        using QCol = QualifiedColumn;

        // Select all unique ids from the results table, and their highest ordered match.
        // The goal is a statement like this:
        //  SELECT m.id, field, match, value, min(sort) from <temp> join manifest on rowid = manifest group by m.id order by t.sort
        // Through the "group by m.id", we will only ever have one row per id, and the "min(sort)" returns us one of the rows that matched
        // through the earliest search.  We also order by the sort value to have the earliest search matches first in the list
        StatementBuilder builder;
        builder.Select().
            Column(QCol(ManifestTable::TableName(), IdTable::ValueName())).
            Column(QCol(tempTableAlias, s_SearchResultsTable_MatchField)).
            Column(QCol(tempTableAlias, s_SearchResultsTable_MatchType)).
            Column(QCol(tempTableAlias, s_SearchResultsTable_MatchValue)).
            Column(Aggregate::Min, QCol(tempTableAlias, s_SearchResultsTable_SortValue)).
        From(GetQualifiedName()).As(tempTableAlias).
            Join(ManifestTable::TableName()).On(QCol(tempTableAlias, s_SearchResultsTable_Manifest), QCol(ManifestTable::TableName(), SQLite::RowIDName)).
            GroupBy(QCol(ManifestTable::TableName(), IdTable::ValueName())).OrderBy(QCol(tempTableAlias, s_SearchResultsTable_SortValue));

        SQLite::Statement select = builder.Prepare(m_connection);

        ISQLiteIndex::SearchResult result;
        while (select.Step())
        {
            if (limit && result.Matches.size() >= limit)
            {
                break;
            }

            result.Matches.emplace_back(select.GetColumn<SQLite::rowid_t>(0), 
                PackageMatchFilter(select.GetColumn<PackageMatchField>(1), select.GetColumn<MatchType>(2), select.GetColumn<std::string>(3)));
        }

        result.Truncated = (select.GetState() != SQLite::Statement::State::Completed);

        return result;
    }

    std::vector<int> SearchResultsTable::BuildSearchStatement(SQLite::Builder::StatementBuilder& builder, PackageMatchField field, MatchType match) const
    {
        return BuildSearchStatement(builder, field, s_SearchResultsTable_SubSelect_ManifestAlias, s_SearchResultsTable_SubSelect_ValueAlias, MatchUsesLike(match));
    }

    std::vector<int> SearchResultsTable::BuildSearchStatement(
        SQLite::Builder::StatementBuilder& builder,
        PackageMatchField field,
        std::string_view manifestAlias,
        std::string_view valueAlias,
        bool useLike) const
    {
        switch (field)
        {
        case PackageMatchField::Id:
            return ManifestTable::BuildSearchStatement<IdTable>(builder, manifestAlias, valueAlias, useLike);
        case PackageMatchField::Name:
            return ManifestTable::BuildSearchStatement<NameTable>(builder, manifestAlias, valueAlias, useLike);
        case PackageMatchField::Moniker:
            return ManifestTable::BuildSearchStatement<MonikerTable>(builder, manifestAlias, valueAlias, useLike);
        case PackageMatchField::Tag:
            return ManifestTable::BuildSearchStatement<TagsTable>(builder, manifestAlias, valueAlias, useLike);
        case PackageMatchField::Command:
            return ManifestTable::BuildSearchStatement<CommandsTable>(builder, manifestAlias, valueAlias, useLike);
        default:
            return {};
        }
    }

    bool SearchResultsTable::MatchUsesLike(MatchType match)
    {
        return (match != MatchType::Exact);
    }

    void SearchResultsTable::BindStatementForMatchType(SQLite::Statement& statement, MatchType match, int bindIndex, std::string_view value)
    {
        std::string valueToUse;

        if (MatchUsesLike(match))
        {
            valueToUse = SQLite::EscapeStringForLike(value);
        }
        else
        {
            valueToUse = value;
        }

        switch (match)
        {
        case AppInstaller::Repository::MatchType::StartsWith:
            valueToUse += '%';
            break;
        case AppInstaller::Repository::MatchType::Substring:
            valueToUse = "%"s + valueToUse + '%';
            break;
        default:
            // No changes required for others.
            break;
        }

        statement.Bind(bindIndex, valueToUse);
    }

    void SearchResultsTable::BindStatementForMatchType(SQLite::Statement& statement, const PackageMatchFilter& filter, const std::vector<int>& bindIndex)
    {
        // TODO: Implement these more complex match types
        if (filter.Type == MatchType::Wildcard || filter.Type == MatchType::Fuzzy || filter.Type == MatchType::FuzzySubstring)
        {
            AICLI_LOG(Repo, Verbose, << "Specific match type not implemented, skipping: " << MatchTypeToString(filter.Type));
            return;
        }

        BindStatementForMatchType(statement, filter.Type, bindIndex[0], filter.Value);
    }
}
