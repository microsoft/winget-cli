// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteWrapper.h"

#define THROW_IF_SQLITE_FAILED(_statement_) \
    do { \
        int _sqliteReturnValue = _statement_; \
        if (_sqliteReturnValue != SQLITE_OK) \
        { \
            throw SQLiteException(_sqliteReturnValue); \
        } \
    } while (0,0);

namespace AppInstaller::Repository::SQLite
{
    namespace
    {
        class SQLiteErrorCategory : public std::error_category
        {
            const char* name() const noexcept override { return "sqlite"; }
            std::string message(int error) const override { return sqlite3_errstr(error); }
        };
    }

    namespace details
    {
        void ParameterSpecifics<std::string>::Bind(sqlite3_stmt* stmt, int index, const std::string& v)
        {
            THROW_IF_SQLITE_FAILED(sqlite3_bind_text(stmt, index, v.c_str(), v.size() + 1, SQLITE_TRANSIENT));
        }

        std::string ParameterSpecifics<std::string>::GetColumn(sqlite3_stmt* stmt, int column)
        {
            return reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
        }
    }

    const std::error_category& SQLiteException::GetCategory() noexcept
    {
        static SQLiteErrorCategory category;
        return category;
    }

    Connection::Connection(const std::string& target, OpenDisposition disposition, OpenFlags flags)
    {
        int resultingFlags = static_cast<int>(disposition) | static_cast<int>(flags);
        THROW_IF_SQLITE_FAILED(sqlite3_open_v2(target.c_str(), &_dbconn, resultingFlags, nullptr));
    }

    Connection Connection::Create(const std::string& target, OpenDisposition disposition, OpenFlags flags)
    {
        Connection result{ target, disposition, flags };
        
        THROW_IF_SQLITE_FAILED(sqlite3_extended_result_codes(result._dbconn, 1));

        return result;
    }

    Connection::~Connection()
    {
        sqlite3_close_v2(_dbconn);
    }
}
