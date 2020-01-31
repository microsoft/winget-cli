// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SQLiteStatementBuilder.h"

namespace AppInstaller::Repository::SQLite::Builder
{
    std::ostream& operator<<(std::ostream& out, const QualifiedColumn& column)
    {
        out << '[' << column.Table << "].[" << column.Column << ']';
        return out;
    }

    StatementBuilder& StatementBuilder::Select(std::string_view column)
    {
        m_stream << "SELECT [" << column << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(std::initializer_list<std::string_view> columns)
    {
        m_stream << "SELECT";
        bool isFirst = true;
        for (const auto& c : columns)
        {
            m_stream << (isFirst ? " [" : ", [") << c << ']';
            isFirst = false;
        }
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(QualifiedColumn column)
    {
        m_stream << "SELECT " << column;
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(std::initializer_list<QualifiedColumn> columns)
    {
        m_stream << "SELECT";
        bool isFirst = true;
        for (const auto& c : columns)
        {
            m_stream << (isFirst ? " " : ", ") << c;
            isFirst = false;
        }
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(details::rowcount_t)
    {
        m_stream << "SELECT COUNT(*)";
        return *this;
    }

    StatementBuilder& StatementBuilder::From(std::string_view table)
    {
        m_stream << " FROM [" << table << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::From(std::initializer_list<std::string_view> table)
    {
        m_stream << " FROM [";
        for (std::string_view t : table)
        {
            m_stream << t;
        }
        m_stream << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Where(std::string_view column)
    {
        m_stream << " WHERE [" << column << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Where(QualifiedColumn column)
    {
        m_stream << " WHERE " << column;
        return *this;
    }

    StatementBuilder& StatementBuilder::Equals(details::unbound_t)
    {
        AppendOpAndBinder(Op::Equals);
        return *this;
    }

    StatementBuilder& StatementBuilder::Equals(std::nullptr_t)
    {
        // This is almost certainly not what you want.
        // In SQL, value = NULL is always false.
        // Use StatementBuilder::IsNull instead.
        THROW_HR(E_NOTIMPL);
    }

    StatementBuilder& StatementBuilder::IsNull()
    {
        m_stream << " IS NULL";
        return *this;
    }

    StatementBuilder& StatementBuilder::And(std::string_view column)
    {
        m_stream << " AND [" << column << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::And(QualifiedColumn column)
    {
        m_stream << " AND " << column;
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(std::string_view table)
    {
        m_stream << " JOIN [" << table << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(std::initializer_list<std::string_view> table)
    {
        m_stream << " JOIN [";
        for (std::string_view t : table)
        {
            m_stream << t;
        }
        m_stream << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::On(QualifiedColumn column1, QualifiedColumn column2)
    {
        m_stream << " ON " << column1 << " = " << column2;
        return *this;
    }

    StatementBuilder& StatementBuilder::Limit(size_t rowCount)
    {
        m_stream << " LIMIT " << rowCount;
        return *this;
    }

    Statement StatementBuilder::Prepare(Connection& connection, bool persistent)
    {
        m_statement = std::make_unique<Statement>(Statement::Create(connection, m_stream.str(), persistent));
        for (const auto& f : m_binders)
        {
            f();
        }
        return std::move(*(m_statement.release()));
    }

    void StatementBuilder::Execute(Connection& connection)
    {
        Prepare(connection).Execute();
    }

    int StatementBuilder::AppendOpAndBinder(Op op)
    {
        switch (op)
        {
        case Op::Equals:
            m_stream << " = ?";
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return m_bindIndex++;
    }
}
