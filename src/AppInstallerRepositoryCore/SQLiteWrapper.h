// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/result_macros.h>
#include <wil/resource.h>
#include <winsqlite/winsqlite3.h>

#include <AppInstallerLogging.h>
#include <AppInstallerLanguageUtilities.h>

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#define SQLITE_MEMORY_DB_CONNECTION_TARGET ":memory:"

using namespace std::string_view_literals;

namespace AppInstaller::Repository::SQLite
{
    // The name of the rowid column in SQLite.
    extern std::string_view RowIDName;

    // The type of a rowid column in code.
    using rowid_t = int64_t;

    // The type to use for blob data.
    using blob_t = std::vector<uint8_t>;

    namespace details
    {
        template <typename T, typename = void>
        struct ParameterSpecificsImpl
        {
            static T& ToLog(T&&)
            {
                static_assert(false, "No type specific override has been supplied");
            }
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
        struct ParameterSpecificsImpl<nullptr_t>
        {
            inline static std::string_view ToLog(nullptr_t) { return "null"sv; }
            static void Bind(sqlite3_stmt* stmt, int index, nullptr_t);
        };

        template <>
        struct ParameterSpecificsImpl<std::string>
        {
            inline static const std::string& ToLog(const std::string& v) { return v; }
            static void Bind(sqlite3_stmt* stmt, int index, const std::string& v);
            static std::string GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <>
        struct ParameterSpecificsImpl<std::string_view>
        {
            inline static const std::string_view& ToLog(const std::string_view& v) { return v; }
            static void Bind(sqlite3_stmt* stmt, int index, std::string_view v);
        };

        template <>
        struct ParameterSpecificsImpl<int>
        {
            inline static int ToLog(int v) { return v; }
            static void Bind(sqlite3_stmt* stmt, int index, int v);
            static int GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <>
        struct ParameterSpecificsImpl<int64_t>
        {
            inline static int64_t ToLog(int64_t v) { return v; }
            static void Bind(sqlite3_stmt* stmt, int index, int64_t v);
            static int64_t GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <>
        struct ParameterSpecificsImpl<bool>
        {
            inline static bool ToLog(bool v) { return v; }
            static void Bind(sqlite3_stmt* stmt, int index, bool v);
            static bool GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <>
        struct ParameterSpecificsImpl<blob_t>
        {
            static std::string ToLog(const blob_t& v);
            static void Bind(sqlite3_stmt* stmt, int index, const blob_t& v);
            static blob_t GetColumn(sqlite3_stmt* stmt, int column);
        };

        template <typename E>
        struct ParameterSpecificsImpl<E, typename std::enable_if_t<std::is_enum_v<E>>>
        {
            static auto ToLog(E v)
            {
                return ToIntegral(v);
            }
            static void Bind(sqlite3_stmt* stmt, int index, E v)
            {
                ParameterSpecificsImpl<std::underlying_type_t<E>>::Bind(stmt, index, ToIntegral(v));
            }
            static E GetColumn(sqlite3_stmt* stmt, int column)
            {
                return ToEnum<E>(ParameterSpecificsImpl<std::underlying_type_t<E>>::GetColumn(stmt, column));
            }
        };

        template <typename T>
        using ParameterSpecifics = ParameterSpecificsImpl<std::decay_t<T>>;
    }

    // A SQLite exception.
    struct SQLiteException : public wil::ResultException
    {
        SQLiteException(int error) : wil::ResultException(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, error)) {}
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

        Connection(Connection&& other) = default;
        Connection& operator=(Connection&& other) = default;

        ~Connection() = default;

        // Enables the ICU integrations on this connection.
        void EnableICU();

        // Gets the last inserted rowid to the database.
        rowid_t GetLastInsertRowID();

        // Gets the count of changed rows for the last executed statement.
        int GetChanges() const;

        operator sqlite3* () const { return m_dbconn.get(); }

    private:
        Connection(const std::string& target, OpenDisposition disposition, OpenFlags flags);

        wil::unique_any<sqlite3*, decltype(sqlite3_close_v2), sqlite3_close_v2> m_dbconn;
    };

    // A SQL statement.
    struct Statement
    {
        static Statement Create(const Connection& connection, const std::string& sql);
        static Statement Create(const Connection& connection, std::string_view sql);
        static Statement Create(const Connection& connection, char const* const sql);

        Statement() = default;

        Statement(const Statement&) = delete;
        Statement& operator=(const Statement&) = delete;

        Statement(Statement&& other) = default;
        Statement& operator=(Statement&& other) = default;

        operator sqlite3_stmt* () const { return m_stmt.get(); }

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
            AICLI_LOG(SQL, Verbose, << "Binding statement #" << m_id << ": " << index << " => " << details::ParameterSpecifics<Value>::ToLog(std::forward<Value>(v)));
            details::ParameterSpecifics<Value>::Bind(m_stmt.get(), index, std::forward<Value>(v));
        }

        // Evaluate the statement; either retrieving the next row or executing some action.
        // Returns true if there is a row of data, or false if there is none.
        // This return value is the equivalent of 'GetState() == State::HasRow' after calling Step.
        bool Step(bool failFastOnError = false);

        // Equivalent to Step, but does not ever expect a result, throwing if one is retrieved.
        void Execute(bool failFastOnError = false);

        // Gets a boolean value that indicates whether the specified column value is null in the current row.
        // The index is 0 based.
        bool GetColumnIsNull(int column);

        // Gets the value of the specified column from the current row.
        // The index is 0 based.
        template <typename Value>
        Value GetColumn(int column)
        {
            THROW_HR_IF(E_BOUNDS, m_state != State::HasRow);
            return details::ParameterSpecifics<Value>::GetColumn(m_stmt.get(), column);
        }

        // Gets the entire row of values from the current row.
        // The values requested *must* be those available starting from the first column, but trailing columns can be omitted.
        template <typename... Values>
        std::tuple<Values...> GetRow()
        {
            return GetRowImpl<Values...>(std::make_integer_sequence<int, sizeof...(Values)>{});
        }

        // Resets the statement state, allowing it to be evaluated again.
        // Note that this does not clear data bindings.
        void Reset();

        // Determines if the statement owns an underlying object.
        operator bool() const { return static_cast<bool>(m_stmt); }

    private:
        Statement(const Connection& connection, std::string_view sql);

        // Helper to receive the integer sequence from the public function.
        // This is equivalent to calling:
        //  for (i = 0 .. count of Values types)
        //      GetColumn<current Value type>(i)
        // Then putting them all into a tuple.
        template <typename... Values, int... I>
        std::tuple<Values...> GetRowImpl(std::integer_sequence<int, I...>)
        {
            THROW_HR_IF(E_BOUNDS, m_state != State::HasRow);
            return std::make_tuple(details::ParameterSpecifics<Values>::GetColumn(m_stmt.get(), I)...);
        }

        size_t m_id = 0;
        wil::unique_any<sqlite3_stmt*, decltype(sqlite3_finalize), sqlite3_finalize> m_stmt;
        State m_state = State::Prepared;
    };

    // A SQLite savepoint.
    struct Savepoint
    {
        // Creates a savepoint, beginning it.
        static Savepoint Create(Connection& connection, std::string name);

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
        DestructionToken m_inProgress = true;
        Statement m_rollbackTo;
        Statement m_release;
    };

    // The escape character used in the EscapeStringForLike function.
    extern std::string_view EscapeCharForLike;

    // Escapes the given input string for passing to a like operation.
    std::string EscapeStringForLike(std::string_view value);
}
