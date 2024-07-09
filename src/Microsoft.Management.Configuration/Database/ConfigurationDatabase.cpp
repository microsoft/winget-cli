// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Database/ConfigurationDatabase.h"
#include "Database/Schema/IConfigurationDatabase.h"
#include <winget/Filesystem.h>
#include "Filesystem.h"

using namespace AppInstaller::SQLite;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        // Use an alternate location for the dev build history.
#ifdef AICLI_DISABLE_TEST_HOOKS
        constexpr std::string_view s_Database_DirectoryName = "History"sv;
#else
        constexpr std::string_view s_Database_DirectoryName = "DevHistory"sv;
#endif

        constexpr std::string_view s_Database_FileName = "config.db"sv;

        #define s_Database_MutexName L"WindowsPackageManager_Configuration_DatabaseMutex"
    }

    ConfigurationDatabase::ConfigurationDatabase() = default;

    ConfigurationDatabase::ConfigurationDatabase(ConfigurationDatabase&&) = default;
    ConfigurationDatabase& ConfigurationDatabase::operator=(ConfigurationDatabase&&) = default;

    ConfigurationDatabase::~ConfigurationDatabase() = default;

    void ConfigurationDatabase::EnsureOpened(bool createIfNeeded)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        if (!std::atomic_load(&m_database))
        {
            std::filesystem::path databaseDirectory = AppInstaller::Filesystem::GetPathTo(PathName::LocalState) / s_Database_DirectoryName;
            std::filesystem::path databaseFile = databaseDirectory / s_Database_FileName;

            {
                wil::unique_mutex databaseMutex;
                databaseMutex.create(s_Database_MutexName);
                auto databaseLock = databaseMutex.acquire();

                if (!std::filesystem::is_regular_file(databaseFile) && createIfNeeded)
                {
                    if (std::filesystem::exists(databaseFile))
                    {
                        std::filesystem::remove_all(databaseDirectory);
                    }

                    std::filesystem::create_directories(databaseDirectory);

                    auto connection = std::make_shared<SQLiteDynamicStorage>(databaseFile, IConfigurationDatabase::GetLatestVersion());
                    auto database = std::shared_ptr{ IConfigurationDatabase::CreateFor(connection) };
                    database->InitializeDatabase();

                    std::atomic_store(&m_connection, connection);
                    std::atomic_store(&m_database, database);
                }
            }

            if (!std::atomic_load(&m_connection))
            {
                std::shared_ptr<SQLiteDynamicStorage> empty;
                auto connection = std::make_shared<SQLiteDynamicStorage>(databaseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
                std::atomic_compare_exchange_strong(&m_connection, &empty, connection);
            }

            if (!std::atomic_load(&m_database))
            {
                std::shared_ptr<IConfigurationDatabase> empty;
                auto database = std::shared_ptr{ IConfigurationDatabase::CreateFor(std::atomic_load(&m_connection), true) };
                std::atomic_compare_exchange_strong(&m_database, &empty, database);
            }
        }
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    std::vector<ConfigurationDatabase::ConfigurationSetPtr> ConfigurationDatabase::GetSetHistory() const
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        if (!database)
        {
            return {};
        }

        auto transaction = BeginTransaction("GetSetHistory", database);
        return database->GetSets();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    void ConfigurationDatabase::WriteSetHistory(const Configuration::ConfigurationSet& configurationSet, bool preferNewHistory)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_POINTER, configurationSet);
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("WriteSetHistory", database);

        std::optional<rowid_t> setRowId = database->GetSetRowId(configurationSet.InstanceIdentifier());

        if (!setRowId && !preferNewHistory)
        {
            // TODO: Use conflict detection code to check for a matching set
        }

        if (setRowId)
        {
            database->UpdateSet(setRowId.value(), configurationSet);
        }
        else
        {
            database->AddSet(configurationSet);
        }

        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    void ConfigurationDatabase::RemoveSetHistory(const Configuration::ConfigurationSet& configurationSet)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_POINTER, configurationSet);

        if (!database)
        {
            return;
        }

        auto transaction = BeginTransaction("RemoveSetHistory", database);

        std::optional<rowid_t> setRowId = database->GetSetRowId(configurationSet.InstanceIdentifier());

        if (!setRowId)
        {
            // TODO: Use conflict detection code to check for a matching set
        }

        if (setRowId)
        {
            database->RemoveSet(setRowId.value());
            std::atomic_load(&m_connection)->SetLastWriteTime();
        }

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }


    void ConfigurationDatabase::AddQueueItem(const Configuration::ConfigurationSet& configurationSet, const std::string& objectName)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_POINTER, configurationSet);
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("AddQueueItem", database);

        database->AddQueueItem(configurationSet.InstanceIdentifier(), objectName);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    void ConfigurationDatabase::SetActiveQueueItem(const std::string& objectName)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("SetActiveQueueItem", database);

        database->SetActiveQueueItem(objectName);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    std::vector<ConfigurationDatabase::QueueItem> ConfigurationDatabase::GetQueueItems() const
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("GetQueueItems", database);

        std::vector<ConfigurationDatabase::QueueItem> result;
        auto queueItems = database->GetQueueItems();
        result.reserve(queueItems.size());

        for (const auto& item : queueItems)
        {
            QueueItem resultItem;
            std::tie(resultItem.SetInstanceIdentifier, resultItem.ObjectName, resultItem.QueuedAt, resultItem.Active) = item;
            result.emplace_back(std::move(resultItem));
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    void ConfigurationDatabase::RemoveQueueItem(const std::string& objectName)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("RemoveQueueItem", database);

        database->RemoveQueueItem(objectName);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    ConfigurationDatabase::TransactionLock ConfigurationDatabase::BeginTransaction(std::string_view name, std::shared_ptr<IConfigurationDatabase>& database) const
    {
        auto connection = std::atomic_load(&m_connection);
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, connection);

        TransactionLock result = connection->TryBeginTransaction(name);

        while (!result)
        {
            {
                auto connectionLock = connection->LockConnection();
                auto newDatabase = std::shared_ptr{ IConfigurationDatabase::CreateFor(connection) };
                if (std::atomic_compare_exchange_strong(&m_database, &database, newDatabase))
                {
                    database = newDatabase;
                }
            }

            result = connection->TryBeginTransaction(name);
        }

        return result;
    }
}
