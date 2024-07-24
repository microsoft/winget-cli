// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/result_macros.h>
#include <wil/resource.h>
#include <winsqlite/winsqlite3.h>

#include <AppInstallerLogging.h>
#include <AppInstallerLanguageUtilities.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#define SQLITE_MEMORY_DB_CONNECTION_TARGET ":memory:"

using namespace std::string_view_literals;

namespace AppInstaller::SQLite
{
    // The name of the rowid column in SQLite.
    extern std::string_view RowIDName;

    // The type of a rowid column in code.
    using rowid_t = int64_t;

    // The type to use for blob data.
    using blob_t = std::vector<uint8_t>;

    namespace details
    {
        template<typename>
        constexpr bool dependent_false = false;

        template <typename T, typename = void>
        struct ParameterSpecificsImpl
        {
            static T& ToLog(T&&)
            {
                static_assert(dependent_false<T>, "No type specific override has been supplied");
            }
            static void Bind(sqlite3_stmt*, int, T&&)
            {
                static_assert(dependent_false<T>, "No type specific override has been supplied");
            }
            static T GetColumn(sqlite3_stmt*, int)
            {
                static_assert(dependent_false<T>, "No type specific override has been supplied");
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

        template <>
        struct ParameterSpecificsImpl<GUID>
        {
            static std::string ToLog(const GUID& v);
            static void Bind(sqlite3_stmt* stmt, int index, const GUID& v);
            static GUID GetColumn(sqlite3_stmt* stmt, int column);
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

        template <typename Opt>
        struct ParameterSpecificsImpl<std::optional<Opt>>
        {
            using Optional = std::optional<Opt>;

            static auto ToLog(const Optional& v)
            {
                std::ostringstream result;
                if (v)
                {
                    result << ParameterSpecificsImpl<Opt>::ToLog(v.value());
                }
                else
                {
                    result << "{null}";
                }
                return std::move(result).str();
            }

            static void Bind(sqlite3_stmt* stmt, int index, const Optional& v)
            {
                if (v)
                {
                    ParameterSpecificsImpl<Opt>::Bind(stmt, index, v.value());
                }
                else
                {
                    ParameterSpecificsImpl<nullptr_t>::Bind(stmt, index, nullptr);
                }
            }

            static Optional GetColumn(sqlite3_stmt* stmt, int column)
            {
                if (sqlite3_column_type(stmt, column) == SQLITE_NULL)
                {
                    return std::nullopt;
                }
                else
                {
                    return ParameterSpecificsImpl<Opt>::GetColumn(stmt, column);
                }
            }
        };

        template <typename T>
        using ParameterSpecifics = ParameterSpecificsImpl<std::decay_t<T>>;

        // Allows the connection to be shared so that it can be closed in some circumstances.
        struct SharedConnection
        {
            // Disables the connection, causing an exception to be thrown by `get`.
            void Disable();

            // Gets the connection object if active.
            sqlite3* Get() const;

            // Gets the connection object for creation.
            sqlite3** GetPtr();

        private:
            std::atomic_bool m_active = true;
            wil::unique_any<sqlite3*, decltype(sqlite3_close_v2), sqlite3_close_v2> m_dbconn;
        };
    }

    // A SQLite exception.
    struct SQLiteException : public wil::ResultException
    {
        SQLiteException(int error) : wil::ResultException(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_SQLITE, error)) {}
    };

    struct Statement;

    // The connection to a database.
    struct Connection
    {
        friend Statement;

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

        //. Gets the (fixed but arbitrary) identifier for this connection.
        size_t GetID() const;

        // Sets the busy timeout for the connection.
        void SetBusyTimeout(std::chrono::milliseconds timeout);

        // Sets the journal mode.
        // Returns true if successful, false if not.
        // Must be performed outside of a transaction.
        bool SetJournalMode(std::string_view mode);

        operator sqlite3* () const { return m_dbconn->Get(); }

    protected:
        // Gets the shared connection.
        std::shared_ptr<details::SharedConnection> GetSharedConnection() const;

    private:
        Connection(const std::string& target, OpenDisposition disposition, OpenFlags flags);

        size_t m_id = 0;
        std::shared_ptr<details::SharedConnection> m_dbconn;
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
            AICLI_LOG(SQL, Verbose, << "Binding statement #" << m_connectionId << '-' << m_id << ": " << index << " => " << details::ParameterSpecifics<Value>::ToLog(std::forward<Value>(v)));
            details::ParameterSpecifics<Value>::Bind(m_stmt.get(), index, std::forward<Value>(v));
        }

        // Evaluate the statement; either retrieving the next row or executing some action.
        // Returns true if there is a row of data, or false if there is none.
        // This return value is the equivalent of 'GetState() == State::HasRow' after calling Step.
        bool Step(bool closeConnectionOnError = false);

        // Equivalent to Step, but does not ever expect a result, throwing if one is retrieved.
        void Execute(bool closeConnectionOnError = false);

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

        std::shared_ptr<details::SharedConnection> m_dbconn;
        size_t m_connectionId = 0;
        size_t m_id = 0;
        wil::unique_any<sqlite3_stmt*, decltype(sqlite3_finalize), sqlite3_finalize> m_stmt;
        State m_state = State::Prepared;
    };

    // A SQLite transaction.
    // Use as the beginning of a transaction stack, specifically when the transaction will write
    // and the database is in WAL mode.
    struct Transaction
    {
        // Creates a transaction, beginning it.
        static Transaction Create(Connection& connection, std::string name, bool immediateWrite);

        Transaction();

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;

        Transaction(Transaction&&) = default;
        Transaction& operator=(Transaction&&) = default;

        ~Transaction();

        // Rolls back the Transaction.
        void Rollback(bool throwOnError = true);

        // Commits the Transaction.
        void Commit();

    private:
        Transaction(Connection& connection, std::string&& name, bool immediateWrite);

        std::string m_name;
        DestructionToken m_inProgress = true;
        Statement m_rollback;
        Statement m_commit;
    };

    // A SQLite savepoint.
    struct Savepoint
    {
        // Creates a savepoint, beginning it.
        static Savepoint Create(Connection& connection, std::string name);

        Savepoint();

        Savepoint(const Savepoint&) = delete;
        Savepoint& operator=(const Savepoint&) = delete;

        Savepoint(Savepoint&&) = default;
        Savepoint& operator=(Savepoint&&) = default;

        ~Savepoint();

        // Rolls back the Savepoint.
        void Rollback(bool throwOnError = true);

        // Commits the Savepoint.
        void Commit();

    private:
        Savepoint(Connection& connection, std::string&& name);

        std::string m_name;
        DestructionToken m_inProgress = true;
        Statement m_rollbackTo;
        Statement m_release;
    };

    // A SQLite backup operation.
    struct Backup
    {
        // Creates a backup.
        static Backup Create(Connection& destination, const std::string& destinationName, Connection& source, const std::string& sourceName);

        Backup(const Backup&) = delete;
        Backup& operator=(const Backup&) = delete;

        Backup(Backup&&) = default;
        Backup& operator=(Backup&&) = default;

        // Performs some or all of the backup.
        // Returns true if the backup is completed, false if not.
        bool Step(int pages = -1);

    private:
        Backup(Connection& destination, const std::string& destinationName, Connection& source, const std::string& sourceName);

        wil::unique_any<sqlite3_backup*, decltype(sqlite3_backup_finish), sqlite3_backup_finish> m_backup;
    };

    // The escape character used in the EscapeStringForLike function.
    extern std::string_view EscapeCharForLike;

    // Escapes the given input string for passing to a like operation.
    std::string EscapeStringForLike(std::string_view value);
}
