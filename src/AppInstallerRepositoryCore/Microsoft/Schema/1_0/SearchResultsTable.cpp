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

        constexpr std::string_view s_SearchResultsTable_SubSelect_TableAlias = "valueTable"sv;
        constexpr std::string_view s_SearchResultsTable_SubSelect_ManifestAlias = "m"sv;
        constexpr std::string_view s_SearchResultsTable_SubSelect_ValueAlias = "v"sv;

        void ExecuteStatementForMatchType(SQLite::Statement& statement, MatchType match, int bindIndex, bool escapeValueForLike, std::string_view value)
        {
            // TODO: Implement these more complex match types
            if (match == MatchType::Wildcard || match == MatchType::Fuzzy || match == MatchType::FuzzySubstring)
            {
                AICLI_LOG(Repo, Verbose, << "Specific match type not implemented, skipping: " << MatchTypeToString(match));
                return;
            }

            std::string_view valueToUse = value;
            std::string escapedValue;
            if (escapeValueForLike)
            {
                escapedValue = SQLite::EscapeStringForLike(value);
                valueToUse = escapedValue;
            }

            if (match == MatchType::Substring)
            {
                escapedValue = "%"s + std::string(valueToUse) + '%';
                valueToUse = escapedValue;
            }

            statement.Bind(bindIndex, valueToUse);

            statement.Execute();
        }
    }

    SearchResultsTable::SearchResultsTable(SQLite::Connection& connection) :
        m_connection(connection)
    {
        using namespace SQLite::Builder;

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

        InitDropStatement(m_connection);
    }

    void SearchResultsTable::SearchOnField(ApplicationMatchField field, MatchType match, std::string_view value)
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
            Value(field).
            Value(match).
            Column(QualifiedColumn(s_SearchResultsTable_SubSelect_TableAlias, s_SearchResultsTable_SubSelect_ValueAlias)).
            Value(sortOrdinal).
            Value(false).
        From().BeginParenthetical();

        bool useLike = (match != MatchType::Exact);
        int bindIndex = 0;

        switch (field)
        {
        case ApplicationMatchField::Id:
            bindIndex = ManifestTable::BuildSearchStatement<IdTable>(builder, s_SearchResultsTable_SubSelect_ManifestAlias, s_SearchResultsTable_SubSelect_ValueAlias, useLike);
            break;
        case ApplicationMatchField::Name:
            bindIndex = ManifestTable::BuildSearchStatement<NameTable>(builder, s_SearchResultsTable_SubSelect_ManifestAlias, s_SearchResultsTable_SubSelect_ValueAlias, useLike);
            break;
        case ApplicationMatchField::Moniker:
            bindIndex = ManifestTable::BuildSearchStatement<MonikerTable>(builder, s_SearchResultsTable_SubSelect_ManifestAlias, s_SearchResultsTable_SubSelect_ValueAlias, useLike);
            break;
        case ApplicationMatchField::Tag:
            bindIndex = ManifestTable::BuildSearchStatement<TagsTable>(builder, s_SearchResultsTable_SubSelect_ManifestAlias, s_SearchResultsTable_SubSelect_ValueAlias, useLike);
            break;
        case ApplicationMatchField::Command:
            bindIndex = ManifestTable::BuildSearchStatement<CommandsTable>(builder, s_SearchResultsTable_SubSelect_ManifestAlias, s_SearchResultsTable_SubSelect_ValueAlias, useLike);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        builder.EndParenthetical().As(s_SearchResultsTable_SubSelect_TableAlias);

        SQLite::Statement statement = builder.Prepare(m_connection);
        ExecuteStatementForMatchType(statement, match, bindIndex, useLike, value);
    }

    void SearchResultsTable::RemoveDuplicateManifestRows()
    {

    }

    void SearchResultsTable::PrepareToFilter()
    {

    }

    void SearchResultsTable::FilterOnField(ApplicationMatchField field, MatchType match, std::string_view value)
    {
        UNREFERENCED_PARAMETER(field);
        UNREFERENCED_PARAMETER(match);
        UNREFERENCED_PARAMETER(value);
    }

    void SearchResultsTable::CompleteFilter()
    {

    }

    std::vector<std::pair<SQLite::rowid_t, ApplicationMatchFilter>> SearchResultsTable::GetSearchResults(size_t limit)
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

        if (limit)
        {
            builder.Limit(limit);
        }

        SQLite::Statement select = builder.Prepare(m_connection);

        std::vector<std::pair<SQLite::rowid_t, ApplicationMatchFilter>> result;
        while (select.Step())
        {
            result.emplace_back(select.GetColumn<SQLite::rowid_t>(0), 
                ApplicationMatchFilter(select.GetColumn<ApplicationMatchField>(1), select.GetColumn<MatchType>(2), select.GetColumn<std::string>(3)));
        }
        return result;
    }
}
