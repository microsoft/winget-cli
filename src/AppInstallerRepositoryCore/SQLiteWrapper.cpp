// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteWrapper.h"

#include <wil/result_macros.h>

// TODO: Invoke the wil error handling callback to log the error
#define THROW_SQLITE(_error_) \
    do { \
        auto _throw_sqlite_exc = SQLiteException(_error_); \
        throw _throw_sqlite_exc; \
    } while (0,0)

#define THROW_IF_SQLITE_FAILED(_statement_) \
    do { \
        int _tisf_sqliteReturnValue = _statement_; \
        if (_tisf_sqliteReturnValue != SQLITE_OK) \
        { \
            THROW_SQLITE(_tisf_sqliteReturnValue); \
        } \
    } while (0,0)

namespace AppInstaller::Repository::SQLite
{
    namespace
    {
        class SQLiteErrorCategory : public std::error_category
        {
            const char* name() const noexcept override { return "sqlite"; }
            std::string message(int error) const override { return sqlite3_errstr(error); }
        };

        size_t GetNextStatementId()
        {
            static std::atomic_size_t statementId(0);
            return ++statementId;
        }
    }

    namespace details
    {
        void ParameterSpecificsImpl<std::string>::Bind(sqlite3_stmt* stmt, int index, const std::string& v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_text64(stmt, index, v.c_str(), v.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
        }

        std::string ParameterSpecificsImpl<std::string>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
        }

        void ParameterSpecificsImpl<int>::Bind(sqlite3_stmt* stmt, int index, int v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_int(stmt, index, v));
        }

        int ParameterSpecificsImpl<int>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return sqlite3_column_int(stmt, column);
        }
    }

    const std::error_category& SQLiteException::GetCategory() noexcept
    {
        static SQLiteErrorCategory category;
        return category;
    }

    Connection::Connection(const std::string& target, OpenDisposition disposition, OpenFlags flags)
    {
        AICLI_LOG(SQL, Info, << "Opening SQLite connection: '" << target << "' [" << std::hex << static_cast<int>(disposition) << ", " << std::hex << static_cast<int>(flags) << "]");
        int resultingFlags = static_cast<int>(disposition) | static_cast<int>(flags);
        THROW_IF_SQLITE_FAILED(sqlite3_open_v2(target.c_str(), &m_dbconn, resultingFlags, nullptr));
    }

    Connection Connection::Create(const std::string& target, OpenDisposition disposition, OpenFlags flags)
    {
        Connection result{ target, disposition, flags };
        
        THROW_IF_SQLITE_FAILED(sqlite3_extended_result_codes(result.m_dbconn, 1));

        return result;
    }

    Connection::~Connection()
    {
        sqlite3_close_v2(m_dbconn);
    }

    Statement::Statement(Connection& connection, const std::string& sql, bool persistent)
    {
        m_id = GetNextStatementId();
        AICLI_LOG(SQL, Verbose, << "Preparing statement #" << m_id << ": " << sql);
        // SQL string size should include the null terminator (https://www.sqlite.org/c3ref/prepare.html)
        THROW_IF_SQLITE_FAILED(sqlite3_prepare_v3(connection, sql.c_str(), static_cast<int>(sql.size() + 1), (persistent ? SQLITE_PREPARE_PERSISTENT : 0), &m_stmt, nullptr));
    }

    Statement Statement::Create(Connection& connection, const std::string& sql, bool persistent)
    {
        return { connection, sql, persistent };
    }

    Statement::~Statement()
    {
        sqlite3_finalize(m_stmt);
    }

    bool Statement::Step(bool failFastOnError)
    {
        AICLI_LOG(SQL, Verbose, << "Stepping statement #" << m_id);
        int result = sqlite3_step(m_stmt);

        if (result == SQLITE_ROW)
        {
            AICLI_LOG(SQL, Verbose, << "Statement #" << m_id << " has data");
            m_state = State::HasRow;
            return true;
        }
        else if (result == SQLITE_DONE)
        {
            AICLI_LOG(SQL, Verbose, << "Statement #" << m_id << " has completed");
            m_state = State::Completed;
            return false;
        }
        else
        {
            m_state = State::Error;
            if (failFastOnError)
            {
                FAIL_FAST_MSG("Critical SQL statement failed");
            }
            else
            {
                THROW_SQLITE(result);
            }
        }
    }

    void Statement::Reset()
    {
        AICLI_LOG(SQL, Verbose, << "Reset statement #" << m_id);
        // Ignore return value from reset, as if it is an error, it was the error from the last call to step.
        sqlite3_reset(m_stmt);
        m_state = State::Prepared;
    }

    Savepoint::Savepoint(Connection& connection, std::string&& name) :
        m_name(std::move(name))
    {
        using namespace std::string_literals;

        Statement begin = Statement::Create(connection, "SAVEPOINT ["s + name + "]");
        m_rollback = Statement::Create(connection, "ROLLBACK TO ["s + name + "]", true);
        m_commit = Statement::Create(connection, "RELEASE ["s + name + "]", true);

        AICLI_LOG(SQL, Info, << "Begin savepoint: " << m_name);
        begin.Step();
    }

    Savepoint Savepoint::Create(Connection& connection, std::string&& name)
    {
        return { connection, std::move(name) };
    }

    Savepoint::~Savepoint()
    {
        Rollback();
    }

    void Savepoint::Rollback()
    {
        if (m_inProgress)
        {
            AICLI_LOG(SQL, Info, << "Roll back savepoint: " << m_name);
            m_rollback.Step(true);
            m_inProgress = false;
        }
    }

    void Savepoint::Commit()
    {
        if (m_inProgress)
        {
            AICLI_LOG(SQL, Info, << "Commit savepoint: " << m_name);
            m_commit.Step(true);
            m_inProgress = false;
        }
    }
}
