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
        if (!m_database)
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

                    m_connection = std::make_shared<SQLiteDynamicStorage>(databaseFile, IConfigurationDatabase::GetLatestVersion());
                    m_database = IConfigurationDatabase::CreateFor(m_connection);
                    m_database->InitializeDatabase();
                }
            }

            if (!m_database && std::filesystem::is_regular_file(databaseFile))
            {
                m_connection = std::make_shared<SQLiteDynamicStorage>(databaseFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
                m_database = IConfigurationDatabase::CreateFor(m_connection);
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
        if (!m_database)
        {
            return {};
        }

        auto transaction = BeginTransaction("GetSetHistory");
        return m_database->GetSets();
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
        THROW_HR_IF_NULL(E_POINTER, configurationSet);
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, m_database);

        auto transaction = BeginTransaction("WriteSetHistory");

        std::optional<rowid_t> setRowId = m_database->GetSetRowId(configurationSet.InstanceIdentifier());

        if (!setRowId && !preferNewHistory)
        {
            // TODO: Use conflict detection code to check for a matching set
        }

        if (setRowId)
        {
            m_database->UpdateSet(setRowId.value(), configurationSet);
        }
        else
        {
            m_database->AddSet(configurationSet);
        }

        m_connection->SetLastWriteTime();

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
        THROW_HR_IF_NULL(E_POINTER, configurationSet);

        if (!m_database)
        {
            return;
        }

        auto transaction = BeginTransaction("RemoveSetHistory");

        std::optional<rowid_t> setRowId = m_database->GetSetRowId(configurationSet.InstanceIdentifier());

        if (!setRowId)
        {
            // TODO: Use conflict detection code to check for a matching set
        }

        if (setRowId)
        {
            m_database->RemoveSet(setRowId.value());
            m_connection->SetLastWriteTime();
        }

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    ConfigurationDatabase::TransactionLock ConfigurationDatabase::BeginTransaction(std::string_view name) const
    {
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, m_connection);

        TransactionLock result = m_connection->TryBeginTransaction(name);

        while (!result)
        {
            {
                auto connectionLock = m_connection->LockConnection();
                m_database = IConfigurationDatabase::CreateFor(m_connection);
            }

            result = m_connection->TryBeginTransaction(name);
        }

        return result;
    }
}
