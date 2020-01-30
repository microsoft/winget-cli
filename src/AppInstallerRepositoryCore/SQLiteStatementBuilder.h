// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"

#include <functional>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

namespace AppInstaller::Repository::SQLite::Builder
{
    namespace details
    {
        // A sentinel type to indicate the the value is not to be bound by the builder.
        struct unbound_t {};
    }

    // A qualified column reference.
    struct QualifiedColumn
    {
        std::string_view Table;
        std::string_view Column;

        explicit QualifiedColumn(std::string_view column) : Column(column) {}
        explicit QualifiedColumn(std::string_view table, std::string_view column) : Table(table), Column(column) {}
    };

    // Pass this value to indicate that the caller will bind the value later.
    __declspec_selectany_ details::unbound_t Unbound;

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

        // Indicate the table that the statement will be operating on.
        StatementBuilder& From(std::string_view table);

        // Begin a filter clause on the given column.
        StatementBuilder& Where(std::string_view column);
        StatementBuilder& Where(QualifiedColumn column);

        // Indicate the operation of the filter clause.
        template <typename ValueType>
        StatementBuilder& Equals(const ValueType& value)
        {
            AppendOpAndBinder(Op::Equals);
            AddBindFunctor(value);
            return *this;
        }
        StatementBuilder& Equals(details::unbound_t);

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
        void AppendOpAndBinder(Op op);

        // Adds a functor to our list that will bind the given value.
        template <typename ValueType>
        void AddBindFunctor(const ValueType& value)
        {
            int binderIndex = m_bindIndex++;
            m_binders.emplace_back([this, binderIndex, &value]() { this->m_statement->Bind(binderIndex, value); });
        }

        std::ostringstream m_stream;
        std::unique_ptr<Statement> m_statement;
        // Because binding values starts at 1
        int m_bindIndex = 1;
        std::vector<std::function<void()>> m_binders;
    };
}
