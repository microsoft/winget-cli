// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/result_macros.h>
#include <winsqlite/winsqlite3.h>

#include <string>
#include <stdexcept>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>

namespace AppInstaller::Repository::SQLite
{
    namespace details
    {
        template <typename T>
        struct ParameterSpecificsImpl
        {
            static void Bind(sqlite3_stmt*, int, T&&)
            {
                static_assert(false, "No type specific override has been supplied");
            }
            static T GetColumn(sqlite3_stmt*, int)
            {
                static_assert(false, "No type specific override has been supplied");
            }
        };

        template <>
        struct ParameterSpecificsImpl<std::string>
        {
            static void Bind(sqlite3_stmt* stmt, int index, const std::string& v);
            static std::string GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <>
        struct ParameterSpecificsImpl<int>
        {
            static void Bind(sqlite3_stmt* stmt, int index, int v);
            static int GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <typename T>
        using ParameterSpecifics = ParameterSpecificsImpl<std::decay_t<T>>;
    }

    // A SQLite exception.
    struct SQLiteException : public std::system_error
    {
        SQLiteException(int error) : std::system_error(error, GetCategory()) {}

        static const std::error_category& GetCategory() noexcept;
    };

    // The connection to a database.
    struct Connection
    {
        // The disposition for opening a database connection.
        enum class OpenDisposition : int
        {
            // Open existing database for reading.
            ReadOnly = SQLITE_OPEN_READONLY,
            // Open existing database for reading and writing.
            ReadWrite = SQLITE_OPEN_READWRITE,
            // Create new database for reading and writing.
            Create = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        };

        // Flags for opening a database connection.
        enum class OpenFlags : int
        {
            // No flags specified.
            None = 0,
            // Indicate that the target can be a URI.
            Uri = SQLITE_OPEN_URI,
        };

        static Connection Create(const std::string& target, OpenDisposition disposition, OpenFlags flags = OpenFlags::None);

        Connection() = default;

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        Connection(Connection&& other) noexcept { std::swap(m_dbconn, other.m_dbconn); }
        Connection& operator=(Connection&& other) noexcept { std::swap(m_dbconn, other.m_dbconn); return *this; }

        ~Connection();

        operator sqlite3* () const { return m_dbconn; }

    private:
        Connection(const std::string& target, OpenDisposition disposition, OpenFlags flags);

        sqlite3* m_dbconn = nullptr;
    };

    // A SQL statement.
    struct Statement
    {
        static Statement Create(Connection& connection, const std::string& sql, bool persistent = false);

        Statement() = default;

        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        Statement(Statement&& other) noexcept { std::swap(m_stmt, other.m_stmt); std::swap(m_state, other.m_state); }
        Statement& operator=(Statement&& other) noexcept { std::swap(m_stmt, other.m_stmt); std::swap(m_state, other.m_state); return *this; }

        ~Statement();

        operator sqlite3_stmt* () const { return m_stmt; }

        // The state of the statement.
        enum class State
        {
            // The statement has been prepared, but not evaluated.
            Prepared = 0,
            // The statement has a row available for reading.
            HasRow = 1,
            // The statement has been completed.
            Completed = 2,
            // The statement has resulted in an error.
            Error = 3,
        };

        // Gets the current state of the statement.
        State GetState() const { return m_state; }

        // Bind parameters to the statement.
        // The index is 1 based.
        template <typename Value>
        void Bind(int index, Value&& v)
        {
            details::ParameterSpecifics<Value>::Bind(m_stmt, index, std::forward<Value>(v));
        }

        // Evaluate the statement; either retrieving the next row or executing some action.
        // Returns true if there is a row of data, or false if there is none.
        // This return value is the equivalent of 'GetState() == State::HasRow' after calling Step.
        bool Step(bool failFastOnError = false);

        // Gets the value of the specified column from the current row.
        // The index is 0 based.
        template <typename Value>
        Value GetColumn(int column)
        {
            THROW_HR_IF(E_BOUNDS, m_state != State::HasRow);
            return details::ParameterSpecifics<Value>::GetColumn(m_stmt, column);
        }

        // Gets the entire row of values from the current row.
        // The values requested *must* be those available starting from the first column, but trailing columns can be ommitted.
        template <typename... Values>
        std::tuple<Values...> GetRow()
        {
            return GetRowImpl<Values...>(std::make_integer_sequence<int, sizeof...(Values)>{});
        }

        // Resets the statement state, allowing it to be evaluated again.
        void Reset();

    private:
        Statement(Connection& connection, const std::string& sql, bool persistent);

        // Helper to receive the integer sequence from the public function.
        // This is equivalent to calling:
        //  for (i = 0 .. count of Values types)
        //      GetColumn<current Value type>(i)
        // Then putting them all into a tuple.
        template <typename... Values, int... I>
        std::tuple<Values...> GetRowImpl(std::integer_sequence<int, I...>)
        {
            THROW_HR_IF(E_BOUNDS, m_state != State::HasRow);
            return std::make_tuple(details::ParameterSpecifics<Values>::GetColumn(m_stmt, I)...);
        }

        size_t m_id = 0;
        sqlite3_stmt* m_stmt = nullptr;
        State m_state = State::Prepared;
    };

    // A SQLite savepoint.
    struct Savepoint
    {
        // Creates a savepoint, beginning it.
        static Savepoint Create(Connection& connection, std::string&& name);

        Savepoint(const Savepoint&) = delete;
        Savepoint& operator=(const Savepoint&) = delete;

        Savepoint(Savepoint&&) = default;
        Savepoint& operator=(Savepoint&&) = default;

        ~Savepoint();

        // Rolls back the Savepoint.
        void Rollback();

        // Commits the Savepoint.
        void Commit();

    private:
        Savepoint(Connection& connection, std::string&& name);

        std::string m_name;
        bool m_inProgress = true;
        Statement m_rollback;
        Statement m_commit;
    };
}
