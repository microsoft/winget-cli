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

    namespace
    {
        void OutputColumns(std::ostream& out, std::string_view op, std::string_view column)
        {
            out << op << '[' << column << ']';
        }

        void OutputColumns(std::ostream& out, std::string_view op, std::initializer_list<std::string_view> columns)
        {
            out << op;
            bool isFirst = true;
            for (const auto& c : columns)
            {
                out << (isFirst ? "[" : ", [") << c << ']';
                isFirst = false;
            }
        }

        void OutputColumns(std::ostream& out, std::string_view op, QualifiedColumn column)
        {
            out << op << column;
        }

        void OutputColumns(std::ostream& out, std::string_view op, std::initializer_list<QualifiedColumn> columns)
        {
            out << op;
            bool isFirst = true;
            for (const auto& c : columns)
            {
                out << (isFirst ? "" : ", ") << c;
                isFirst = false;
            }
        }

        // Use to output operation and table name, such as " FROM [table]"
        void OutputOperationAndTable(std::ostream& out, std::string_view op, std::string_view table)
        {
            out << op << " [" << table << ']';
        }

        void OutputOperationAndTable(std::ostream& out, std::string_view op, std::initializer_list<std::string_view> table)
        {
            out << op << " [";
            for (std::string_view t : table)
            {
                out << t;
            }
            out << ']';
        }
    }

    StatementBuilder& StatementBuilder::Select(std::string_view column)
    {
        OutputColumns(m_stream, "SELECT ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(std::initializer_list<std::string_view> columns)
    {
        OutputColumns(m_stream, "SELECT ", columns);
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(QualifiedColumn column)
    {
        OutputColumns(m_stream, "SELECT ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(std::initializer_list<QualifiedColumn> columns)
    {
        OutputColumns(m_stream, "SELECT ", columns);
        return *this;
    }

    StatementBuilder& StatementBuilder::Select(details::rowcount_t)
    {
        m_stream << "SELECT COUNT(*)";
        return *this;
    }

    StatementBuilder& StatementBuilder::From(std::string_view table)
    {
        OutputOperationAndTable(m_stream, " FROM", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::From(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, " FROM", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Where(std::string_view column)
    {
        OutputColumns(m_stream, " WHERE ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::Where(QualifiedColumn column)
    {
        OutputColumns(m_stream, " WHERE ", column);
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
        OutputColumns(m_stream, " AND ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::And(QualifiedColumn column)
    {
        OutputColumns(m_stream, " AND ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(std::string_view table)
    {
        OutputOperationAndTable(m_stream, " JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, " JOIN", table);
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

    StatementBuilder& StatementBuilder::InsertInto(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "INSERT INTO", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::InsertInto(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "INSERT INTO", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Columns(std::string_view column)
    {
        OutputColumns(m_stream, "(", column);
        m_stream << ')';
        return *this;
    }

    StatementBuilder& StatementBuilder::Columns(std::initializer_list<std::string_view> columns)
    {
        OutputColumns(m_stream, "(", columns);
        m_stream << ')';
        return *this;
    }

    StatementBuilder& StatementBuilder::Columns(QualifiedColumn column)
    {
        OutputColumns(m_stream, "(", column);
        m_stream << ')';
        return *this;
    }

    StatementBuilder& StatementBuilder::Columns(std::initializer_list<QualifiedColumn> columns)
    {
        OutputColumns(m_stream, "(", columns);
        m_stream << ')';
        return *this;
    }

    Statement StatementBuilder::Prepare(Connection& connection, bool persistent)
    {
        Statement result = Statement::Create(connection, m_stream.str(), persistent);
        for (const auto& f : m_binders)
        {
            f(result);
        }
        return result;
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

    int StatementBuilder::AppendValuesAndBinders(size_t count)
    {
        m_stream << " VALUES (";
        for (size_t i = 0; i < count; ++i)
        {
            m_stream << (i == 0 ? "?" : ", ?");
        }
        m_stream << ')';

        int result = m_bindIndex;
        m_bindIndex += static_cast<int>(count);
        return result;
    }
}
