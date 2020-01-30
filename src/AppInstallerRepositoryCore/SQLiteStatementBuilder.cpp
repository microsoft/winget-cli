// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteStatementBuilder.h"

namespace AppInstaller::Repository::SQLite::Builder
{
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
        m_stream << "SELECT [" << column.Table << "].[" << column.Column << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(std::initializer_list<QualifiedColumn> columns)
    {
        m_stream << "SELECT";
        bool isFirst = true;
        for (const auto& c : columns)
        {
            m_stream << (isFirst ? " [" : ", [") << c.Table << "].[" << c.Column << ']';
            isFirst = false;
        }
        return *this;
    }

    StatementBuilder& StatementBuilder::From(std::string_view table)
    {
        m_stream << " FROM [" << table << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Where(std::string_view column)
    {
        m_stream << " WHERE [" << column << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Where(QualifiedColumn column)
    {
        m_stream << " WHERE [" << column.Table << "].[" << column.Column << ']';
        return *this;
    }

    StatementBuilder& StatementBuilder::Equals(details::unbound_t)
    {
        AppendOpAndBinder(Op::Equals);
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

    void StatementBuilder::AppendOpAndBinder(Op op)
    {
        switch (op)
        {
        case Op::Equals:
            m_stream << " = ?";
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }
}
