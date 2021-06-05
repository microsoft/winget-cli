// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteWrapper.h"
#include "ICU/SQLiteICU.h"

#include <wil/result_macros.h>

using namespace std::string_view_literals;

// Enable this to have all Statement constructions output the associated query plan.
#define WINGET_SQLITE_EXPLAIN_QUERY_PLAN_ENABLED 0

#if WINGET_SQLITE_EXPLAIN_QUERY_PLAN_ENABLED
#include <stack>
#endif

#define THROW_SQLITE(_error_) \
    do { \
        int _ts_sqliteReturnValue = _error_; \
        THROW_EXCEPTION_MSG(SQLiteException(_ts_sqliteReturnValue), sqlite3_errstr(_ts_sqliteReturnValue)); \
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
    std::string_view RowIDName = "rowid"sv;

    namespace
    {
        size_t GetNextStatementId()
        {
            static std::atomic_size_t statementId(0);
            return ++statementId;
        }
    }

    namespace details
    {
        void ParameterSpecificsImpl<nullptr_t>::Bind(sqlite3_stmt* stmt, int index, nullptr_t)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_null(stmt, index));
        }

        void ParameterSpecificsImpl<std::string>::Bind(sqlite3_stmt* stmt, int index, const std::string& v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_text64(stmt, index, v.c_str(), v.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
        }

        std::string ParameterSpecificsImpl<std::string>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
        }

        void ParameterSpecificsImpl<std::string_view>::Bind(sqlite3_stmt* stmt, int index, std::string_view v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_text64(stmt, index, v.data(), v.size(), SQLITE_TRANSIENT, SQLITE_UTF8));
        }

        void ParameterSpecificsImpl<int>::Bind(sqlite3_stmt* stmt, int index, int v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_int(stmt, index, v));
        }

        int ParameterSpecificsImpl<int>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return sqlite3_column_int(stmt, column);
        }

        void ParameterSpecificsImpl<int64_t>::Bind(sqlite3_stmt* stmt, int index, int64_t v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_int64(stmt, index, v));
        }

        int64_t ParameterSpecificsImpl<int64_t>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return sqlite3_column_int64(stmt, column);
        }

        void ParameterSpecificsImpl<bool>::Bind(sqlite3_stmt* stmt, int index, bool v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_int(stmt, index, (v ? 1 : 0)));
        }

        bool ParameterSpecificsImpl<bool>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return (sqlite3_column_int(stmt, column) != 0);
        }

        std::string ParameterSpecificsImpl<blob_t>::ToLog(const blob_t& v)
        {
            std::ostringstream strstr;
            strstr << "blob[" << v.size() << "]";
            return strstr.str();
        }

        void ParameterSpecificsImpl<blob_t>::Bind(sqlite3_stmt* stmt, int index, const blob_t& v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_blob64(stmt, index, v.data(), v.size(), SQLITE_TRANSIENT));
        }

        blob_t ParameterSpecificsImpl<blob_t>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            const blob_t::value_type* blobPtr = reinterpret_cast<const blob_t::value_type *>(sqlite3_column_blob(stmt, column));
            if (blobPtr)
            {
                int blobBytes = sqlite3_column_bytes(stmt, column);
                return blob_t{ blobPtr, blobPtr + blobBytes };
            }
            else
            {
                return {};
            }
        }
    }

    Connection::Connection(const std::string& target, OpenDisposition disposition, OpenFlags flags)
    {
        AICLI_LOG(SQL, Info, << "Opening SQLite connection: '" << target << "' [" << std::hex << static_cast<int>(disposition) << ", " << std::hex << static_cast<int>(flags) << "]");
        // Always force connection serialization until we determine that there are situations where it is not needed
        int resultingFlags = static_cast<int>(disposition) | static_cast<int>(flags) | SQLITE_OPEN_FULLMUTEX;
        THROW_IF_SQLITE_FAILED(sqlite3_open_v2(target.c_str(), &m_dbconn, resultingFlags, nullptr));
    }

    Connection Connection::Create(const std::string& target, OpenDisposition disposition, OpenFlags flags)
    {
        Connection result{ target, disposition, flags };
        
        THROW_IF_SQLITE_FAILED(sqlite3_extended_result_codes(result.m_dbconn.get(), 1));

        return result;
    }

    void Connection::EnableICU()
    {
        AICLI_LOG(SQL, Verbose, << "Enabling ICU");
        THROW_IF_SQLITE_FAILED(sqlite3IcuInit(m_dbconn.get()));
    }

    rowid_t Connection::GetLastInsertRowID()
    {
        return sqlite3_last_insert_rowid(m_dbconn.get());
    }

    int Connection::GetChanges() const
    {
        return sqlite3_changes(m_dbconn.get());
    }

    Statement::Statement(const Connection& connection, std::string_view sql)
    {
        m_id = GetNextStatementId();
        AICLI_LOG(SQL, Verbose, << "Preparing statement #" << m_id << ": " << sql);
        // SQL string size should include the null terminator (https://www.sqlite.org/c3ref/prepare.html)
        assert(sql.data()[sql.size()] == '\0');
        THROW_IF_SQLITE_FAILED(sqlite3_prepare_v2(connection, sql.data(), static_cast<int>(sql.size() + 1), &m_stmt, nullptr));
    }

#if WINGET_SQLITE_EXPLAIN_QUERY_PLAN_ENABLED
#define WINGET_SQLITE_EXPLAIN_QUERY_PLAN(_connection_,_sql_) \
    std::string _explainStatementSQL_ = "EXPLAIN QUERY PLAN "; \
    _explainStatementSQL_.append(_sql_); \
    try { \
        Statement _explainStatement_(_connection_,_explainStatementSQL_); \
        LogExplainQueryPlanResult(_sql_, _explainStatement_); \
    } catch(...) {}

    void LogExplainQueryPlanResult(std::string_view sql, Statement& plan)
    {
        bool outputHeader = true;
        std::stack<int> parents;

        while (plan.Step())
        {
            if (outputHeader)
            {
                AICLI_LOG(SQL, Info, << "Query plan for: " << sql);
                outputHeader = false;
            }

            int id = plan.GetColumn<int>(0);
            int parent = plan.GetColumn<int>(1);

            while (!parents.empty() && parents.top() != parent)
            {
                parents.pop();
            }

            AICLI_LOG(SQL, Info, << "|-" << std::string(parents.size() * 2, '-') << ' ' << plan.GetColumn<std::string>(3));

            parents.push(id);
        }
    }
#else
#define WINGET_SQLITE_EXPLAIN_QUERY_PLAN(_connection_,_sql_)
#endif

    Statement Statement::Create(const Connection& connection, const std::string& sql)
    {
        WINGET_SQLITE_EXPLAIN_QUERY_PLAN(connection, sql);
        return { connection, { sql.c_str(), sql.size() } };
    }

    Statement Statement::Create(const Connection& connection, std::string_view sql)
    {
        WINGET_SQLITE_EXPLAIN_QUERY_PLAN(connection, sql);
        // We need the statement to be null terminated, and the only way to guarantee that with a string_view is to construct a string copy.
        return Create(connection, std::string(sql));
    }

    Statement Statement::Create(const Connection& connection, char const* const sql)
    {
        WINGET_SQLITE_EXPLAIN_QUERY_PLAN(connection, sql);
        return { connection, sql };
    }

    bool Statement::Step(bool failFastOnError)
    {
        AICLI_LOG(SQL, Verbose, << "Stepping statement #" << m_id);
        int result = sqlite3_step(m_stmt.get());

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

    void Statement::Execute(bool failFastOnError)
    {
        THROW_HR_IF(E_UNEXPECTED, Step(failFastOnError));
    }

    bool Statement::GetColumnIsNull(int column)
    {
        int type = sqlite3_column_type(m_stmt.get(), column);
        return type == SQLITE_NULL;
    }

    void Statement::Reset()
    {
        AICLI_LOG(SQL, Verbose, << "Reset statement #" << m_id);
        // Ignore return value from reset, as if it is an error, it was the error from the last call to step.
        sqlite3_reset(m_stmt.get());
        m_state = State::Prepared;
    }

    Savepoint::Savepoint(Connection& connection, std::string&& name) :
        m_name(std::move(name))
    {
        using namespace std::string_literals;

        Statement begin = Statement::Create(connection, "SAVEPOINT ["s + m_name + "]");
        m_rollbackTo = Statement::Create(connection, "ROLLBACK TO ["s + m_name + "]");
        m_release = Statement::Create(connection, "RELEASE ["s + m_name + "]");

        AICLI_LOG(SQL, Verbose, << "Begin savepoint: " << m_name);
        begin.Step();
    }

    Savepoint Savepoint::Create(Connection& connection, std::string name)
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
            AICLI_LOG(SQL, Verbose, << "Roll back savepoint: " << m_name);
            m_rollbackTo.Step(true);
            // 'ROLLBACK TO' *DOES NOT* remove the savepoint from the transaction stack.
            // In order to remove it, we must RELEASE. Since we just invoked a ROLLBACK TO
            // this should have the effect of 'committing' nothing.
            m_release.Step(true);
            m_inProgress = false;
        }
    }

    void Savepoint::Commit()
    {
        if (m_inProgress)
        {
            AICLI_LOG(SQL, Verbose, << "Commit savepoint: " << m_name);
            m_release.Step(true);
            m_inProgress = false;
        }
    }

    std::string_view EscapeCharForLike = "'"sv;

    std::string EscapeStringForLike(std::string_view value)
    {
        constexpr char singleChar = '_';
        constexpr char multiChar = '%';
        char escapeChar = EscapeCharForLike[0];

        std::string result;
        result.reserve(value.length());

        for (char c : value)
        {
            if (c == singleChar || c == multiChar || c == escapeChar)
            {
                result.append(1, escapeChar);
            }
            result.append(1, c);
        }

        return result;
    }
}
