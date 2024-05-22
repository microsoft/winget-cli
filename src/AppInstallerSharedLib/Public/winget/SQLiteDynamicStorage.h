// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteStorageBase.h>
#include <memory>

namespace AppInstaller::SQLite
{
    // Type the allows for the schema version of the underlying storage to be changed dynamically.
    struct SQLiteDynamicStorage : public SQLiteStorageBase
    {
        // Creates a new database with the given schema version.
        SQLiteDynamicStorage(const std::string& target, const Version& version);

        // Opens an existing database with the given disposition.
        SQLiteDynamicStorage(const std::string& filePath, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& file = {});

        // Implicit conversion to a connection object for convenience.
        operator Connection& ();
        operator const Connection& () const;

        // Must be kept alive to ensure consistent schema view and exclusive use of the owned connection.
        struct TransactionLock
        {
            TransactionLock(std::mutex& mutex, const Connection& connection);

        private:
            std::lock_guard<std::mutex> m_lock;
            Savepoint m_transaction;
        };

        // Acquires the connection lock and begins a transaction on the database.
        // If the returned result is empty, the schema version has changed and the caller must handle this.
        std::unique_ptr<TransactionLock> TryBeginTransaction();
    };
}
