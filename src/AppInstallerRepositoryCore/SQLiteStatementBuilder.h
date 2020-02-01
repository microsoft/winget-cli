// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string_view>
#include <vector>

namespace AppInstaller::Repository::SQLite::Builder
{
    namespace details
    {
        // Sentinel types to indicate special cases to the builder.
        struct unbound_t {};
        struct rowcount_t {};
    }

    // Pass this value to indicate that the caller will bind the value later.
    __declspec_selectany_ details::unbound_t Unbound;

    // Pass this value to indicate that the number of rows is to be selected.
    __declspec_selectany_ details::rowcount_t RowCount;

    // A qualified column reference.
    struct QualifiedColumn
    {
        std::string_view Table;
        std::string_view Column;

        explicit QualifiedColumn(std::string_view column) : Column(column) {}
        explicit QualifiedColumn(std::string_view table, std::string_view column) : Table(table), Column(column) {}
    };

    // A class that aids in building SQL statements in a more expressive manner than simple strings.
    struct StatementBuilder
    {
        StatementBuilder() = default;

        StatementBuilder(const StatementBuilder&) = default;
        StatementBuilder& operator=(const StatementBuilder&) = default;

        StatementBuilder(StatementBuilder&&) = default;
        StatementBuilder& operator=(StatementBuilder&&) = default;

        // Begin a select statement for the given columns.
        StatementBuilder& Select(std::string_view column);
        StatementBuilder& Select(std::initializer_list<std::string_view> columns);
        StatementBuilder& Select(QualifiedColumn column);
        StatementBuilder& Select(std::initializer_list<QualifiedColumn> columns);
        StatementBuilder& Select(details::rowcount_t);

        // Indicate the table that the statement will be operating on.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& From(std::string_view table);
        StatementBuilder& From(std::initializer_list<std::string_view> table);

        // Begin a filter clause on the given column.
        StatementBuilder& Where(std::string_view column);
        StatementBuilder& Where(QualifiedColumn column);

        // Indicate the operation of the filter clause.
        template <typename ValueType>
        StatementBuilder& Equals(const ValueType& value)
        {
            AddBindFunctor(AppendOpAndBinder(Op::Equals), value);
            return *this;
        }
        template <typename ValueType>
        StatementBuilder& Equals(const std::optional<ValueType>& value)
        {
            if (value)
            {
                AddBindFunctor(AppendOpAndBinder(Op::Equals), value.value());
                return *this;
            }
            else
            {
                return IsNull();
            }
        }
        StatementBuilder& Equals(details::unbound_t);
        StatementBuilder& Equals(std::nullptr_t);

        StatementBuilder& IsNull();

        // Operators for combining filter clauses.
        StatementBuilder& And(std::string_view column);
        StatementBuilder& And(QualifiedColumn column);

        // Begin a join clause.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& Join(std::string_view table);
        StatementBuilder& Join(std::initializer_list<std::string_view> table);

        // Set the join constraint.
        StatementBuilder& On(QualifiedColumn column1, QualifiedColumn column2);

        // Limits the result set to the given number of rows.
        StatementBuilder& Limit(size_t rowCount);

        // Prepares and returns the statement, applying any bindings that were requested.
        Statement Prepare(Connection& connection, bool persistent = false);

        // A convenience function that prepares, binds, and then executes a statement that does not return rows.
        void Execute(Connection& connection);

    private:
        enum class Op
        {
            Equals
        };

        // Appends given the operation.
        int AppendOpAndBinder(Op op);

        // Adds a functor to our list that will bind the given value.
        template <typename ValueType>
        void AddBindFunctor(int binderIndex, const ValueType& value)
        {
            m_binders.emplace_back([this, binderIndex, &value]() { this->m_statement->Bind(binderIndex, value); });
        }

        std::ostringstream m_stream;
        std::unique_ptr<Statement> m_statement;
        // Because binding values starts at 1
        int m_bindIndex = 1;
        std::vector<std::function<void()>> m_binders;
    };
}
