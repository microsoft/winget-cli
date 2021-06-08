// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SQLiteStatementBuilder.h"

namespace AppInstaller::Repository::SQLite::Builder
{
    std::ostream& operator<<(std::ostream& out, const QualifiedColumn& column)
    {
        if (!column.Table.empty())
        {
            out << '[' << column.Table << "].";
        }
        out << '[' << column.Column << ']';
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const QualifiedTable& table)
    {
        if (!table.Schema.empty())
        {
            out << '[' << table.Schema << "].";
        }
        out << '[' << table.Table << ']';
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const details::SubBuilder& column)
    {
        out << column.GetString();
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

        void OutputColumns(std::ostream& out, std::string_view op, const QualifiedColumn& column)
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

        void OutputColumns(std::ostream& out, std::string_view op, std::initializer_list<details::SubBuilder> columns)
        {
            out << op;
            bool isFirst = true;
            for (const auto& c : columns)
            {
                out << (isFirst ? "" : ", ") << c;
                isFirst = false;
            }
        }

        void OutputAggregate(std::ostream& out, Aggregate op)
        {
            out << ' ';
            switch (op)
            {
            case Aggregate::Min:
                out << "MIN";
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        void OutputColumns(std::ostream& out, Aggregate op, std::string_view column)
        {
            OutputAggregate(out, op);
            out << "([" << column << "])";
        }

        void OutputColumns(std::ostream& out, Aggregate op, const QualifiedColumn& column)
        {
            OutputAggregate(out, op);
            out << '(' << column << ')';
        }

        // Use to output operation and table name, such as " FROM [table]"
        void OutputOperationAndTable(std::ostream& out, std::string_view op, std::string_view table)
        {
            out << op << " [" << table << ']';
        }

        void OutputOperationAndTable(std::ostream& out, std::string_view op, QualifiedTable table)
        {
            out << op << table;
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

        void OutputType(std::ostream& out, Type type)
        {
            out << ' ';
            switch (type)
            {
            case Type::Int:
                out << "INT";
                break;
            case Type::Int64:
                out << "INT64";
                break;
            case Type::Text:
                out << "TEXT";
                break;
            case Type::Blob:
                out << "BLOB";
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }
    }

    IntegerPrimaryKey::IntegerPrimaryKey()
    {
        m_stream << SQLite::RowIDName << " INTEGER PRIMARY KEY";
    }

    IntegerPrimaryKey& IntegerPrimaryKey::AutoIncrement(bool isTrue)
    {
        if (isTrue)
        {
            m_stream << " AUTOINCREMENT";
        }
        return *this;
    }

    ColumnBuilder::ColumnBuilder(std::string_view column, Type type)
    {
        OutputColumns(m_stream, "", column);
        OutputType(m_stream, type);
    }

    ColumnBuilder& ColumnBuilder::NotNull(bool isTrue)
    {
        if (isTrue)
        {
            m_stream << " NOT NULL";
        }
        return *this;
    }

    ColumnBuilder& ColumnBuilder::Default(int64_t value)
    {
        m_stream << " DEFAULT " << value;
        return *this;
    }

    ColumnBuilder& ColumnBuilder::Unique(bool isTrue)
    {
        if (isTrue)
        {
            m_stream << " UNIQUE";
        }
        return *this;
    }

    ColumnBuilder& ColumnBuilder::PrimaryKey(bool isTrue)
    {
        if (isTrue)
        {
            m_stream << " PRIMARY KEY";
        }
        return *this;
    }

    PrimaryKeyBuilder::PrimaryKeyBuilder(std::initializer_list<std::string_view> columns)
    {
        OutputColumns(m_stream, "PRIMARY KEY(", columns);
        m_stream << ')';
        m_needsClosing = false;
    }

    PrimaryKeyBuilder::PrimaryKeyBuilder()
    {
        m_stream << "PRIMARY KEY(";
    }

    PrimaryKeyBuilder& PrimaryKeyBuilder::Column(std::string_view column)
    {
        if (m_isFirst)
        {
            m_isFirst = false;
        }
        else
        {
            m_stream << ", ";
        }
        OutputColumns(m_stream, "", column);
        return *this;
    }

    PrimaryKeyBuilder::operator details::SubBuilder()
    {
        if (m_needsClosing)
        {
            m_stream << ')';
            m_needsClosing = false;
        }
        return { m_stream.str() };
    }

    StatementBuilder& StatementBuilder::Select()
    {
        m_stream << "SELECT ";
        m_needsComma = false;
        return *this;
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

    StatementBuilder& StatementBuilder::Select(const QualifiedColumn& column)
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

    StatementBuilder& StatementBuilder::From()
    {
        m_stream << " FROM ";
        return *this;
    }

    StatementBuilder& StatementBuilder::From(std::string_view table)
    {
        OutputOperationAndTable(m_stream, " FROM", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::From(QualifiedTable table)
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

    StatementBuilder& StatementBuilder::Where(const QualifiedColumn& column)
    {
        OutputColumns(m_stream, " WHERE ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::Equals(details::unbound_t)
    {
        AppendOpAndBinder(Op::Equals);
        return *this;
    }

    StatementBuilder& StatementBuilder::LikeWithEscape(std::string_view value)
    {
        AddBindFunctor(AppendOpAndBinder(Op::Like), EscapeStringForLike(value));
        return Escape(EscapeCharForLike);
    }

    StatementBuilder& StatementBuilder::Like(details::unbound_t)
    {
        AppendOpAndBinder(Op::Like);
        return *this;
    }

    StatementBuilder& StatementBuilder::LiteralColumn(std::string_view value)
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        AddBindFunctor(AppendOpAndBinder(Op::Literal), value);
        m_needsComma = true;
        return *this;
    }

    StatementBuilder& StatementBuilder::Escape(std::string_view escapeChar)
    {
        THROW_HR_IF(E_INVALIDARG, escapeChar.length() != 1);
        AddBindFunctor(AppendOpAndBinder(Op::Escape), escapeChar);
        return *this;
    }

    StatementBuilder& StatementBuilder::Equals(std::nullptr_t)
    {
        // This is almost certainly not what you want.
        // In SQL, value = NULL is always false.
        // Use StatementBuilder::IsNull instead.
        THROW_HR(E_NOTIMPL);
    }

    StatementBuilder& StatementBuilder::Not()
    {
        m_stream << " NOT";
        return *this;
    }

    StatementBuilder& StatementBuilder::In()
    {
        m_stream << " IN";
        return *this;
    }

    StatementBuilder& StatementBuilder::IsNull(bool isNull)
    {
        m_stream << " IS " << (isNull ? "" : "NOT ") << "NULL";
        return *this;
    }

    StatementBuilder& StatementBuilder::And(std::string_view column)
    {
        OutputColumns(m_stream, " AND ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::And(const QualifiedColumn& column)
    {
        OutputColumns(m_stream, " AND ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(std::string_view table)
    {
        OutputOperationAndTable(m_stream, " JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, " JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Join(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, " JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::LeftOuterJoin(std::string_view table)
    {
        OutputOperationAndTable(m_stream, " LEFT OUTER JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::LeftOuterJoin(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, " LEFT OUTER JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::LeftOuterJoin(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, " LEFT OUTER JOIN", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::On(const QualifiedColumn& column1, const QualifiedColumn& column2)
    {
        m_stream << " ON " << column1 << " = " << column2;
        return *this;
    }

    StatementBuilder& StatementBuilder::Limit(size_t rowCount)
    {
        m_stream << " LIMIT " << rowCount;
        return *this;
    }

    StatementBuilder& StatementBuilder::GroupBy(std::string_view column)
    {
        OutputColumns(m_stream, " GROUP BY ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::GroupBy(const QualifiedColumn& column)
    {
        OutputColumns(m_stream, " GROUP BY ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::OrderBy(std::string_view column)
    {
        OutputColumns(m_stream, " ORDER BY ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::OrderBy(const QualifiedColumn& column)
    {
        OutputColumns(m_stream, " ORDER BY ", column);
        return *this;
    }

    StatementBuilder& StatementBuilder::InsertInto(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "INSERT INTO", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::InsertInto(QualifiedTable table)
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

    StatementBuilder& StatementBuilder::Columns(const QualifiedColumn& column)
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

    StatementBuilder& StatementBuilder::Columns(std::initializer_list<details::SubBuilder> columns)
    {
        OutputColumns(m_stream, "(", columns);
        m_stream << ')';
        return *this;
    }

    StatementBuilder& StatementBuilder::BeginColumns()
    {
        m_stream << '(';
        m_needsComma = false;
        return *this;
    }

    StatementBuilder& StatementBuilder::Column(std::string_view column)
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        OutputColumns(m_stream, "", column);
        m_needsComma = true;
        return *this;
    }

    StatementBuilder& StatementBuilder::Column(const QualifiedColumn& column)
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        OutputColumns(m_stream, "", column);
        m_needsComma = true;
        return *this;
    }

    StatementBuilder& StatementBuilder::Column(Aggregate aggOp, std::string_view column)
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        OutputColumns(m_stream, aggOp, column);
        m_needsComma = true;
        return *this;
    }

    StatementBuilder& StatementBuilder::Column(Aggregate aggOp, const QualifiedColumn& column)
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        OutputColumns(m_stream, aggOp, column);
        m_needsComma = true;
        return *this;
    }

    StatementBuilder& StatementBuilder::Column(const details::SubBuilder& column)
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        m_stream << column;
        m_needsComma = true;
        return *this;
    }

    StatementBuilder& StatementBuilder::EndColumns()
    {
        m_stream << ')';
        m_needsComma = false;
        return *this;
    }

    StatementBuilder& StatementBuilder::BeginValues()
    {
        m_stream << " VALUES (";
        m_needsComma = false;
        return *this;
    }

    StatementBuilder& StatementBuilder::EndValues()
    {
        m_stream << ')';
        m_needsComma = false;
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateTable(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "CREATE TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateTable(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "CREATE TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateTable(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "CREATE TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::AlterTable(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "ALTER TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::AlterTable(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "ALTER TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::AlterTable(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "ALTER TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Add(std::string_view column, Type type)
    {
        m_stream << " ADD " << column;
        OutputType(m_stream, type);
        return *this;
    }

    StatementBuilder& StatementBuilder::DropTable(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "DROP TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DropTable(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "DROP TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DropTable(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "DROP TABLE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateIndex(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "CREATE INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateIndex(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "CREATE INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateIndex(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "CREATE INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateUniqueIndex(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "CREATE UNIQUE INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateUniqueIndex(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "CREATE UNIQUE INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::CreateUniqueIndex(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "CREATE UNIQUE INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DropIndex(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "DROP INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DropIndex(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "DROP INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DropIndex(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "DROP INDEX", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::On(std::string_view table)
    {
        OutputOperationAndTable(m_stream, " ON", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::On(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, " ON", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DeleteFrom(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "DELETE FROM", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DeleteFrom(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "DELETE FROM", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::DeleteFrom(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "DELETE FROM", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Update(std::string_view table)
    {
        OutputOperationAndTable(m_stream, "UPDATE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Update(QualifiedTable table)
    {
        OutputOperationAndTable(m_stream, "UPDATE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Update(std::initializer_list<std::string_view> table)
    {
        OutputOperationAndTable(m_stream, "UPDATE", table);
        return *this;
    }

    StatementBuilder& StatementBuilder::Set()
    {
        m_stream << " SET ";
        m_needsComma = false;
        return *this;
    }

    StatementBuilder& StatementBuilder::Vacuum()
    {
        m_stream << "VACUUM";
        return *this;
    }

    StatementBuilder& StatementBuilder::BeginParenthetical()
    {
        m_stream << '(';
        return *this;
    }

    StatementBuilder& StatementBuilder::EndParenthetical()
    {
        m_stream << ')';
        return *this;
    }

    StatementBuilder& StatementBuilder::As(std::string_view alias)
    {
        OutputOperationAndTable(m_stream, " AS", alias);
        return *this;
    }

    Statement StatementBuilder::Prepare(const Connection& connection)
    {
        Statement result = Statement::Create(connection, m_stream.str());
        for (const auto& f : m_binders)
        {
            f(result);
        }
        return result;
    }

    void StatementBuilder::Execute(const Connection& connection)
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
        case Op::Like:
            m_stream << " LIKE ?";
            break;
        case Op::Escape:
            m_stream << " ESCAPE ?";
            break;
        case Op::Literal:
            m_stream << " ?";
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

    int StatementBuilder::AppendValueAndBinder()
    {
        if (m_needsComma)
        {
            m_stream << ", ";
        }
        m_stream << '?';
        m_needsComma = true;
        return m_bindIndex++;
    }
}
