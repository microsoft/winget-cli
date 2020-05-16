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

        // Class for intake from external functions.
        struct SubBuilder
        {
            SubBuilder(std::string&& s) : m_string(std::move(s)) {}

            SubBuilder(const SubBuilder&) = default;
            SubBuilder& operator=(const SubBuilder&) = default;

            SubBuilder(SubBuilder&&) noexcept = default;
            SubBuilder& operator=(SubBuilder&&) noexcept = default;

            const std::string& GetString() const { return m_string; }

        protected:
            std::string m_string;
        };

        // Base class for all sub-builders.
        struct SubBuilderBase
        {
            SubBuilderBase() = default;

            SubBuilderBase(const SubBuilderBase&) = default;
            SubBuilderBase& operator=(const SubBuilderBase&) = default;

            SubBuilderBase(SubBuilderBase&&) noexcept = default;
            SubBuilderBase& operator=(SubBuilderBase&&) noexcept = default;

            virtual operator SubBuilder() { return { m_stream.str() }; }

        protected:
            std::ostringstream m_stream;
        };
    }

    // Pass this value to indicate that the caller will bind the value later.
    __declspec_selectany_ details::unbound_t Unbound;

    // Pass this value to indicate that the number of rows is to be selected.
    __declspec_selectany_ details::rowcount_t RowCount;

    // A qualified table reference.
    struct QualifiedTable
    {
        std::string_view Schema;
        std::string_view Table;

        explicit QualifiedTable(std::string_view table) : Table(table) {}
        explicit QualifiedTable(std::string_view schema, std::string_view table) : Schema(schema), Table(table) {}
    };

    // A qualified column reference.
    struct QualifiedColumn
    {
        std::string_view Table;
        std::string_view Column;

        explicit QualifiedColumn(std::string_view column) : Column(column) {}
        explicit QualifiedColumn(std::string_view table, std::string_view column) : Table(table), Column(column) {}
    };

    // SQLite types as an enum.
    enum class Type
    {
        Int,
        Bool = Int,
        Int64,
        RowId = Int64,
        Text,
    };

    // Aggregate functions.
    enum class Aggregate
    {
        Min
    };

    // Helper used when creating a table.
    struct ColumnBuilder : public details::SubBuilderBase
    {
        // Specify the column name and type when creating the builder.
        ColumnBuilder(std::string_view column, Type type);

        ColumnBuilder(const ColumnBuilder&) = default;
        ColumnBuilder& operator=(const ColumnBuilder&) = default;

        ColumnBuilder(ColumnBuilder&&) noexcept = default;
        ColumnBuilder& operator=(ColumnBuilder&&) noexcept = default;

        // Indicate that the column is not able to be null.
        // Allow for data driven construction with input value.
        ColumnBuilder& NotNull(bool isTrue = true);

        // Indicate the default value for the column.
        // Note that a default value is not considered constant if it is bound,
        // so this function directly places the incoming value into the SQL statement.
        ColumnBuilder& Default(int64_t value);

        // Indicate that the column is unique.
        // Allow for data driven construction with input value.
        ColumnBuilder& Unique(bool isTrue = true);

        // Indicate that the column is the primary key.
        // Allow for data driven construction with input value.
        ColumnBuilder& PrimaryKey(bool isTrue = true);
    };

    // Helper used to specify a primary key with multiple columns.
    struct PrimaryKeyBuilder : public details::SubBuilderBase
    {
        PrimaryKeyBuilder();
        PrimaryKeyBuilder(std::initializer_list<std::string_view> columns);

        PrimaryKeyBuilder(const PrimaryKeyBuilder&) = default;
        PrimaryKeyBuilder& operator=(const PrimaryKeyBuilder&) = default;

        PrimaryKeyBuilder(PrimaryKeyBuilder&&) noexcept = default;
        PrimaryKeyBuilder& operator=(PrimaryKeyBuilder&&) noexcept = default;

        virtual operator details::SubBuilder() override;

        // Add a column to the primary key.
        PrimaryKeyBuilder& Column(std::string_view column);

    private:
        bool m_isFirst = true;
        bool m_needsClosing = true;
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
        StatementBuilder& Select();
        StatementBuilder& Select(std::string_view column);
        StatementBuilder& Select(std::initializer_list<std::string_view> columns);
        StatementBuilder& Select(const QualifiedColumn& column);
        StatementBuilder& Select(std::initializer_list<QualifiedColumn> columns);
        StatementBuilder& Select(details::rowcount_t);

        // Indicate the table that the statement will be operating on.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& From();
        StatementBuilder& From(std::string_view table);
        StatementBuilder& From(QualifiedTable table);
        StatementBuilder& From(std::initializer_list<std::string_view> table);

        // Begin a filter clause on the given column.
        StatementBuilder& Where(std::string_view column);
        StatementBuilder& Where(const QualifiedColumn& column);

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

        StatementBuilder& LikeWithEscape(std::string_view value);
        StatementBuilder& Like(details::unbound_t);

        StatementBuilder& Escape(std::string_view escapeChar);

        StatementBuilder& Not();
        StatementBuilder& In();

        StatementBuilder& IsNull();

        // Operators for combining filter clauses.
        StatementBuilder& And(std::string_view column);
        StatementBuilder& And(const QualifiedColumn& column);

        // Begin a join clause.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& Join(std::string_view table);
        StatementBuilder& Join(QualifiedTable table);
        StatementBuilder& Join(std::initializer_list<std::string_view> table);

        // Set the join constraint.
        StatementBuilder& On(const QualifiedColumn& column1, const QualifiedColumn& column2);

        // Specify the grouping to use.
        StatementBuilder& GroupBy(std::string_view column);
        StatementBuilder& GroupBy(const QualifiedColumn& column);

        // Specify the ordering to use.
        StatementBuilder& OrderBy(std::string_view column);
        StatementBuilder& OrderBy(const QualifiedColumn& column);

        // Limits the result set to the given number of rows.
        StatementBuilder& Limit(size_t rowCount);

        // Begin an insert statement for the given table.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& InsertInto(std::string_view table);
        StatementBuilder& InsertInto(QualifiedTable table);
        StatementBuilder& InsertInto(std::initializer_list<std::string_view> table);

        // Set the columns for a statement (typically insert).
        StatementBuilder& Columns(std::string_view column);
        StatementBuilder& Columns(std::initializer_list<std::string_view> columns);
        StatementBuilder& Columns(const QualifiedColumn& column);
        StatementBuilder& Columns(std::initializer_list<QualifiedColumn> columns);

        // Set the columns for a select or create table statement.
        StatementBuilder& Columns(std::initializer_list<details::SubBuilder> columns);
        StatementBuilder& BeginColumns();
        StatementBuilder& Column(std::string_view column);
        StatementBuilder& Column(const QualifiedColumn& column);
        StatementBuilder& Column(Aggregate aggOp, std::string_view column);
        StatementBuilder& Column(Aggregate aggOp, const QualifiedColumn& column);
        StatementBuilder& Column(const details::SubBuilder& column);
        StatementBuilder& EndColumns();

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
        StatementBuilder& BeginValues();
        template <typename ValueType>
        StatementBuilder& Value(const ValueType& value)
        {
            InsertValuesValueBinder(AppendValueAndBinder(), value);
            return *this;
        }
        StatementBuilder& EndValues();

        // Begin a table creation statement.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& CreateTable(std::string_view table);
        StatementBuilder& CreateTable(QualifiedTable table);
        StatementBuilder& CreateTable(std::initializer_list<std::string_view> table);

        // Begin an table deletion statement.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& DropTable(std::string_view table);
        StatementBuilder& DropTable(QualifiedTable table);
        StatementBuilder& DropTable(std::initializer_list<std::string_view> table);

        // Begin an index creation statement.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& CreateIndex(std::string_view table);
        StatementBuilder& CreateIndex(QualifiedTable table);
        StatementBuilder& CreateIndex(std::initializer_list<std::string_view> table);

        // Begin an index deletion statement.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& DropIndex(std::string_view table);
        StatementBuilder& DropIndex(QualifiedTable table);
        StatementBuilder& DropIndex(std::initializer_list<std::string_view> table);

        // Set index target table.
        StatementBuilder& On(std::string_view table);
        StatementBuilder& On(std::initializer_list<std::string_view> table);

        // Begin a delete statement.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& DeleteFrom(std::string_view table);
        StatementBuilder& DeleteFrom(QualifiedTable table);
        StatementBuilder& DeleteFrom(std::initializer_list<std::string_view> table);

        // Begin an update statement.
        // The initializer_list form enables the table name to be constructed from multiple parts.
        StatementBuilder& Update(std::string_view table);
        StatementBuilder& Update(QualifiedTable table);
        StatementBuilder& Update(std::initializer_list<std::string_view> table);

        // Output the set portion of an update statement.
        StatementBuilder& Set();

        // Output the set portion of an update statement.
        StatementBuilder& Vacuum();

        // General purpose functions to begin and end a parenthetical expression.
        StatementBuilder& BeginParenthetical();
        StatementBuilder& EndParenthetical();

        // Assign an alias to the previous item.
        StatementBuilder& As(std::string_view alias);

        // Gets the last bound index.
        // A value of zero indicates that nothing has been bound.
        int GetLastBindIndex() const { return m_bindIndex - 1; }

        // Prepares and returns the statement, applying any bindings that were requested.
        Statement Prepare(Connection& connection);

        // A convenience function that prepares, binds, and then executes a statement that does not return rows.
        void Execute(Connection& connection);

    private:
        enum class Op
        {
            Equals,
            Like,
            Escape
        };

        // Appends given the operation.
        int AppendOpAndBinder(Op op);

        // Appends a set of binders for the values clause of an insert.
        int AppendValuesAndBinders(size_t count);

        // Appends a binder for the values clause of an insert.
        int AppendValueAndBinder();

        // Adds a functor to our list that will bind the given value.
        template <typename ValueType>
        void AddBindFunctor(int binderIndex, const ValueType& value)
        {
            m_binders.emplace_back([binderIndex, value](Statement& s) { s.Bind(binderIndex, value); });
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
        StatementBuilder& InsertValuesValueBinder(int, details::unbound_t)
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
        bool m_needsComma = false;
    };
}
