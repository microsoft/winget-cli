// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/SQLiteDynamicStorage.h"

namespace AppInstaller::SQLite
{
    SQLiteDynamicStorage::SQLiteDynamicStorage(const std::string& target, const Version& version) : SQLiteStorageBase(target, version)
    {
        version.SetSchemaVersion(m_dbconn);
    }

    SQLiteDynamicStorage::SQLiteDynamicStorage(const std::filesystem::path& target, const Version& version) : SQLiteDynamicStorage(target.u8string(), version)
    {}

    SQLiteDynamicStorage::SQLiteDynamicStorage(
        const std::string& filePath,
        SQLiteStorageBase::OpenDisposition disposition,
        Utility::ManagedFile&& file)
        : SQLiteStorageBase(filePath, disposition, std::move(file))
    {}

    SQLiteDynamicStorage::SQLiteDynamicStorage(
        const std::filesystem::path& filePath,
        SQLiteStorageBase::OpenDisposition disposition,
        Utility::ManagedFile&& file)
        : SQLiteDynamicStorage(filePath.u8string(), disposition, std::move(file))
    {}

    SQLiteDynamicStorage::operator Connection& ()
    {
        return m_dbconn;
    }

    SQLiteDynamicStorage::operator const Connection& () const
    {
        return m_dbconn;
    }

    Connection& SQLiteDynamicStorage::GetConnection()
    {
        return m_dbconn;
    }

    const Connection& SQLiteDynamicStorage::GetConnection() const
    {
        return m_dbconn;
    }

    _Acquires_lock_(mutex)
    SQLiteDynamicStorage::TransactionLock::TransactionLock(std::mutex& mutex) :
        m_lock(mutex)
    {
    }

    _Acquires_lock_(mutex)
    SQLiteDynamicStorage::TransactionLock::TransactionLock(std::mutex& mutex, Connection& connection, std::string_view name, bool immediateWrite) :
        m_lock(mutex)
    {
        m_transaction = Transaction::Create(connection, std::string{ name }, immediateWrite);
    }

    void SQLiteDynamicStorage::TransactionLock::Rollback(bool throwOnError)
    {
        m_transaction.Rollback(throwOnError);
    }

    void SQLiteDynamicStorage::TransactionLock::Commit()
    {
        m_transaction.Commit();
    }

    std::unique_ptr<SQLiteDynamicStorage::TransactionLock> SQLiteDynamicStorage::TryBeginTransaction(std::string_view name, bool immediateWrite)
    {
        auto result = std::make_unique<TransactionLock>(*m_interfaceLock, m_dbconn, name, immediateWrite);

        Version currentVersion = Version::GetSchemaVersion(m_dbconn);
        if (currentVersion != m_version)
        {
            m_version = currentVersion;
            result.reset();
        }

        return result;
    }

    std::unique_ptr<SQLiteDynamicStorage::TransactionLock> SQLiteDynamicStorage::LockConnection()
    {
        return std::make_unique<TransactionLock>(*m_interfaceLock);
    }
}
