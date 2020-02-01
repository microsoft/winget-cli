// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include <AppInstallerLanguageUtilities.h>

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

        // Begin an insert statement for the given table.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& InsertInto(std::string_view table);
        StatementBuilder& InsertInto(std::initializer_list<std::string_view> table);

        // Set the columns for a statement (typically insert).
        StatementBuilder& Columns(std::string_view column);
        StatementBuilder& Columns(std::initializer_list<std::string_view> columns);
        StatementBuilder& Columns(QualifiedColumn column);
        StatementBuilder& Columns(std::initializer_list<QualifiedColumn> columns);

        // Add the values clause for an insert statement.
        template <typename... ValueTypes>
        StatementBuilder& Values(const ValueTypes&... values)
        {
            int bindIndexBegin = AppendValuesAndBinders(sizeof...(ValueTypes));
            // Use folding to add a binder for every value, specifically in the order they were given.
            // Do not change this expression without understanding the implications to the bind order.
            // See: https://en.cppreference.com/w/cpp/language/fold for more details.
            (FoldHelper{}, ..., InsertValuesValueBinder(bindIndexBegin++, values));
            return *this;
        }

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

        // Appends a set of binders for the values clause of an insert.
        int AppendValuesAndBinders(size_t count);

        // Adds a functor to our list that will bind the given value.
        template <typename ValueType>
        void AddBindFunctor(int binderIndex, const ValueType& value)
        {
            m_binders.emplace_back([binderIndex, &value](Statement& s) { s.Bind(binderIndex, value); });
        }

        // Helper template for binding incoming values for an insert.
        template <typename ValueType>
        StatementBuilder& InsertValuesValueBinder(int bindIndex, const ValueType& value)
        {
            AddBindFunctor(bindIndex, value);
            return *this;
        }
        template <typename ValueType>
        StatementBuilder& InsertValuesValueBinder(int bindIndex, const std::optional<ValueType>& value)
        {
            if (value)
            {
                AddBindFunctor(bindIndex, value.value());
            }
            else
            {
                AddBindFunctor(bindIndex, nullptr);
            }
            return *this;
        }
        StatementBuilder& InsertValuesValueBinder(int bindIndex, details::unbound_t)
        {
            return *this;
        }
        StatementBuilder& InsertValuesValueBinder(int bindIndex, std::nullptr_t)
        {
            AddBindFunctor(bindIndex, nullptr);
            return *this;
        }

        std::ostringstream m_stream;
        // Because binding values starts at 1
        int m_bindIndex = 1;
        std::vector<std::function<void(Statement&)>> m_binders;
    };
}
