// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SearchResultsTable.h"
#include <winget/SQLiteStatementBuilder.h>

#include "Microsoft/Schema/2_0/PackagesTable.h"
#include "Microsoft/Schema/2_0/TagsTable.h"
#include "Microsoft/Schema/2_0/CommandsTable.h"
#include "Microsoft/Schema/2_0/PackageFamilyNameTable.h"
#include "Microsoft/Schema/2_0/ProductCodeTable.h"
#include "Microsoft/Schema/2_0/UpgradeCodeTable.h"
#include "Microsoft/Schema/2_0/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/2_0/NormalizedPackagePublisherTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;

        constexpr std::string_view s_SearchResultsTable_Package = "package"sv;
        constexpr std::string_view s_SearchResultsTable_MatchField = "field"sv;
        constexpr std::string_view s_SearchResultsTable_MatchType = "match"sv;
        constexpr std::string_view s_SearchResultsTable_MatchValue = "value"sv;
        constexpr std::string_view s_SearchResultsTable_SortValue = "sort"sv;
        constexpr std::string_view s_SearchResultsTable_Filter = "filter"sv;

        constexpr std::string_view s_SearchResultsTable_Index_Suffix = "_i_m"sv;

        constexpr std::string_view s_SearchResultsTable_SubSelect_TableAlias = "valueTable"sv;
        constexpr std::string_view s_SearchResultsTable_SubSelect_PackageAlias = "p"sv;
        constexpr std::string_view s_SearchResultsTable_SubSelect_ValueAlias = "v"sv;
    }

    SearchResultsTable::SearchResultsTable(const SQLite::Connection& connection) :
        m_connection(connection)
    {
        using namespace SQLite::Builder;

        {
            StatementBuilder builder;
            builder.CreateTable(GetQualifiedName()).BeginColumns();

            builder.Column(ColumnBuilder(s_SearchResultsTable_Package, Type::RowId).NotNull());
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
            builder.CreateIndex(indexName).On(GetQualifiedName().Table).Columns(s_SearchResultsTable_Package);

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
        //      SELECT valueTable.p, <field>, <match>, valueTable.v, <sort>, <filter> FROM
        //      (SELECT packages.rowid as p, packages.id as v from packages where packages.id = <value>) AS valueTable
        // Where the subselect is built by the owning table.
        StatementBuilder builder;
        builder.InsertInto(GetQualifiedName()).Select().
            Column(QualifiedColumn(s_SearchResultsTable_SubSelect_TableAlias, s_SearchResultsTable_SubSelect_PackageAlias)).
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
            AICLI_LOG(Repo, Verbose, << "PackageMatchField not supported in this version: " << ToString(filter.Field));
            return;
        }

        builder.EndParenthetical().As(s_SearchResultsTable_SubSelect_TableAlias);

        SQLite::Statement statement = builder.Prepare(m_connection);
        BindStatementForMatchType(statement, filter, bindIndex);
        statement.Execute();
        AICLI_LOG(SQL, Verbose, << "Search found " << m_connection.GetChanges() << " rows");
    }

    void SearchResultsTable::RemoveDuplicatePackageRows()
    {
        using namespace SQLite::Builder;

        // Create a delete statement to leave only one row with a given package.
        // This will arbitrarily choose one of the rows if multiple have the same lowest sort order.
        // The goal is a statement like this:
        //      DELETE from <temp> where rowid not in (
        //          SELECT rowid from (
        //              SELECT rowid, min(sort) from <temp> group by package
        //          )
        //      )
        StatementBuilder builder;
        builder.DeleteFrom(GetQualifiedName()).Where(SQLite::RowIDName).Not().In().BeginParenthetical().
            Select(SQLite::RowIDName).From().BeginParenthetical().
                Select().Column(SQLite::RowIDName).Column(Aggregate::Min, s_SearchResultsTable_SortValue).From(GetQualifiedName()).GroupBy(s_SearchResultsTable_Package).
            EndParenthetical().
        EndParenthetical();

        builder.Execute(m_connection);
        AICLI_LOG(SQL, Verbose, << "Removed " << m_connection.GetChanges() << " duplicate rows");
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
        //      UPDATE <temp> set filter = 1 where package in (
        //          SELECT p from (
        //              SELECT packages.rowid as p, packages.id as v from packages where packages.id = <value>
        //          )
        //      )
        StatementBuilder builder;
        builder.Update(GetQualifiedName()).Set().Column(s_SearchResultsTable_Filter).Equals(true).Where(s_SearchResultsTable_Package).In().BeginParenthetical().
            Select(s_SearchResultsTable_SubSelect_PackageAlias).From().BeginParenthetical();

        // Add the field specific portion
        std::vector<int> bindIndex = BuildSearchStatement(builder, filter.Field, filter.Type);

        if (bindIndex.empty())
        {
            AICLI_LOG(Repo, Verbose, << "PackageMatchField not supported in this version: " << ToString(filter.Field));
            return;
        }

        builder.EndParenthetical().EndParenthetical();

        SQLite::Statement statement = builder.Prepare(m_connection);
        BindStatementForMatchType(statement, filter, bindIndex);
        statement.Execute();
        AICLI_LOG(SQL, Verbose, << "Filter kept " << m_connection.GetChanges() << " rows");
    }

    void SearchResultsTable::CompleteFilter()
    {
        // Delete all unselected values
        SQLite::Builder::StatementBuilder builder;
        builder.DeleteFrom(GetQualifiedName()).Where(s_SearchResultsTable_Filter).Equals(false);

        builder.Execute(m_connection);
        AICLI_LOG(SQL, Verbose, << "Filter deleted " << m_connection.GetChanges() << " rows");
    }

    ISQLiteIndex::SearchResult SearchResultsTable::GetSearchResults(size_t limit)
    {
        using namespace SQLite::Builder;
        using QCol = QualifiedColumn;

        // Select all of the results from the table; it is expected that RemoveDuplicatePackageRows has been called.
        StatementBuilder builder;
        builder.Select({
            s_SearchResultsTable_Package,
            s_SearchResultsTable_MatchField,
            s_SearchResultsTable_MatchType,
            s_SearchResultsTable_MatchValue,
        }).
        From(GetQualifiedName()).OrderBy(s_SearchResultsTable_SortValue);

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
        return BuildSearchStatement(builder, field, s_SearchResultsTable_SubSelect_PackageAlias, s_SearchResultsTable_SubSelect_ValueAlias, MatchUsesLike(match));
    }

    std::vector<int> SearchResultsTable::BuildSearchStatement(
        SQLite::Builder::StatementBuilder& builder,
        PackageMatchField field,
        std::string_view manifestAlias,
        std::string_view valueAlias,
        bool useLike) const
    {
        std::vector<int> result;

        switch (field)
        {
        case PackageMatchField::Id:
            result.push_back(PackagesTable::BuildSearchStatement(builder, PackagesTable::IdColumn::Name, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::Name:
            result.push_back(PackagesTable::BuildSearchStatement(builder, PackagesTable::NameColumn::Name, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::Moniker:
            result.push_back(PackagesTable::BuildSearchStatement(builder, PackagesTable::MonikerColumn::Name, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::Tag:
            result.push_back(TagsTable::BuildSearchStatement(builder, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::Command:
            result.push_back(CommandsTable::BuildSearchStatement(builder, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::PackageFamilyName:
            result.push_back(PackageFamilyNameTable::BuildSearchStatement(builder, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::ProductCode:
            result.push_back(ProductCodeTable::BuildSearchStatement(builder, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::UpgradeCode:
            result.push_back(UpgradeCodeTable::BuildSearchStatement(builder, manifestAlias, valueAlias, useLike));
            break;
        case PackageMatchField::NormalizedNameAndPublisher:
            result = NormalizedPackageNameTable::BuildPairedSearchStatement<NormalizedPackagePublisherTable>(builder, manifestAlias, valueAlias, useLike);
            break;
        }

        return result;
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
            AICLI_LOG(Repo, Verbose, << "Specific match type not implemented, skipping: " << ToString(filter.Type));
            return;
        }

        BindStatementForMatchType(statement, filter.Type, bindIndex[0], filter.Value);

        if (filter.Field == PackageMatchField::NormalizedNameAndPublisher)
        {
            BindStatementForMatchType(statement, filter.Type, bindIndex[1], filter.Additional.value());
        }
    }
}
