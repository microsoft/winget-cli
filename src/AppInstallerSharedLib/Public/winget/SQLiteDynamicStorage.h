// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteStorageBase.h>
#include <concurrencysal.h>
#include <filesystem>
#include <memory>

namespace AppInstaller::SQLite
{
    // Type the allows for the schema version of the underlying storage to be changed dynamically.
    struct SQLiteDynamicStorage : public SQLiteStorageBase
    {
        // Creates a new database with the given schema version.
        SQLiteDynamicStorage(const std::string& target, const Version& version);
        SQLiteDynamicStorage(const std::filesystem::path& target, const Version& version);

        // Opens an existing database with the given disposition.
        SQLiteDynamicStorage(const std::string& filePath, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& file = {});
        SQLiteDynamicStorage(const std::filesystem::path& filePath, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& file = {});

        // Implicit conversion to a connection object for convenience.
        operator Connection& ();
        operator const Connection& () const;
        Connection& GetConnection();
        const Connection& GetConnection() const;

        using SQLiteStorageBase::SetLastWriteTime;

        // Must be kept alive to ensure consistent schema view and exclusive use of the owned connection.
        struct TransactionLock
        {
            _Acquires_lock_(mutex)
            TransactionLock(std::mutex& mutex);

            _Acquires_lock_(mutex)
            TransactionLock(std::mutex& mutex, Connection& connection, std::string_view name, bool immediateWrite);

            // Abandons the transaction and any changes; releases the connection lock.
            void Rollback(bool throwOnError = true);

            // Commits the transaction and releases the connection lock.
            void Commit();

        private:
            std::lock_guard<std::mutex> m_lock;
            Transaction m_transaction;
        };

        // Acquires the connection lock and begins a transaction on the database.
        // If the returned result is empty, the schema version has changed and the caller must handle this.
        std::unique_ptr<TransactionLock> TryBeginTransaction(std::string_view name, bool immediateWrite);

        // Locks the connection for use during the schema upgrade.
        std::unique_ptr<TransactionLock> LockConnection();
    };
}
