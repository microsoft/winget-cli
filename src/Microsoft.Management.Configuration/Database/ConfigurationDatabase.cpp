// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Database/ConfigurationDatabase.h"
#include "Database/Schema/IConfigurationDatabase.h"
#include "ConfigurationUnitResultInformation.h"
#include <AppInstallerStrings.h>
#include <winget/Filesystem.h>
#include "Filesystem.h"

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

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

        std::vector<ConfigurationDatabase::StatusItem> ConvertStatusItems(const std::vector<IConfigurationDatabase::StatusItemTuple>& input)
        {
            std::vector<ConfigurationDatabase::StatusItem> result;

            for (const auto& item : input)
            {
                ConfigurationDatabase::StatusItem statusItem{};
                std::tie(
                    statusItem.ChangeIdentifier,
                    statusItem.ChangeTime,
                    statusItem.SetInstanceIdentifier,
                    statusItem.InQueue,
                    statusItem.UnitInstanceIdentifier,
                    statusItem.State,
                    statusItem.ResultCode,
                    statusItem.ResultDescription,
                    statusItem.ResultDetails,
                    statusItem.ResultSource) = item;
                result.emplace_back(std::move(statusItem));
            }

            return result;
        }
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

    ConfigurationDatabase::ConfigurationSetPtr ConfigurationDatabase::GetSet(const GUID& instanceIdentifier) const
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

        auto transaction = BeginTransaction("GetSet", database);
        return database->GetSet(instanceIdentifier);
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
            std::tie(resultItem.SetInstanceIdentifier, resultItem.ObjectName, resultItem.QueuedAt, resultItem.ProcessId, resultItem.Active) = item;
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

    std::vector<ConfigurationDatabase::StatusItem> ConfigurationDatabase::GetStatusSince(int64_t changeIdentifier) const
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        std::vector<StatusItem> result;

        if (database)
        {
            auto transaction = BeginTransaction("GetStatusSince", database);
            result = ConvertStatusItems(database->GetStatusSince(changeIdentifier));
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    ConfigurationDatabase::StatusBaseline ConfigurationDatabase::GetStatusBaseline() const
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        StatusBaseline result{};

        if (database)
        {
            auto transaction = BeginTransaction("GetStatusBaseline", database);
            auto [changeIdentifier, setStatus] = database->GetStatusBaseline();
            result.ChangeIdentifier = changeIdentifier;
            result.SetStatus = ConvertStatusItems(setStatus);
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    void ConfigurationDatabase::AddListener(const std::string& objectName)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("AddListener", database);

        database->AddListener(objectName);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    void ConfigurationDatabase::RemoveListener(const std::string& objectName)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("RemoveListener", database);

        database->RemoveListener(objectName);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    std::vector<ConfigurationDatabase::StatusChangeListener> ConfigurationDatabase::GetChangeListeners() const
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        std::vector<StatusChangeListener> result;

        if (database)
        {
            auto transaction = BeginTransaction("GetChangeListeners", database);

            for (const auto& item : database->GetChangeListeners())
            {
                StatusChangeListener listener{};
                std::tie(listener.ObjectName, listener.Started, listener.ProcessId) = item;
                result.emplace_back(std::move(listener));
            }
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    void ConfigurationDatabase::UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("UpdateSetState", database);

        database->UpdateSetState(setInstanceIdentifier, state);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    void ConfigurationDatabase::UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("UpdateSetInQueue", database);

        database->UpdateSetInQueue(setInstanceIdentifier, inQueue);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    void ConfigurationDatabase::UpdateUnitState(const guid& setInstanceIdentifier, const com_ptr<implementation::ConfigurationSetChangeData>& changeData)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);

        THROW_HR_IF_NULL(E_NOT_VALID_STATE, database);

        auto transaction = BeginTransaction("UpdateUnitState", database);

        database->UpdateUnitState(setInstanceIdentifier, changeData);
        std::atomic_load(&m_connection)->SetLastWriteTime();

        transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    ConfigurationSetState ConfigurationDatabase::GetSetState(const guid& instanceIdentifier)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        ConfigurationSetState result = ConfigurationSetState::Unknown;

        if (database)
        {
            auto transaction = BeginTransaction("GetSetState", database);
            result = database->GetSetState(instanceIdentifier);
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    std::chrono::system_clock::time_point ConfigurationDatabase::GetSetFirstApply(const guid& instanceIdentifier)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        std::chrono::system_clock::time_point result{};

        if (database)
        {
            auto transaction = BeginTransaction("GetSetFirstApply", database);
            result = database->GetSetFirstApply(instanceIdentifier);
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    std::chrono::system_clock::time_point ConfigurationDatabase::GetSetApplyBegun(const guid& instanceIdentifier)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        std::chrono::system_clock::time_point result{};

        if (database)
        {
            auto transaction = BeginTransaction("GetSetApplyBegun", database);
            result = database->GetSetApplyBegun(instanceIdentifier);
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    std::chrono::system_clock::time_point ConfigurationDatabase::GetSetApplyEnded(const guid& instanceIdentifier)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        std::chrono::system_clock::time_point result{};

        if (database)
        {
            auto transaction = BeginTransaction("GetSetApplyEnded", database);
            result = database->GetSetApplyEnded(instanceIdentifier);
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    ConfigurationUnitState ConfigurationDatabase::GetUnitState(const guid& instanceIdentifier)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        ConfigurationUnitState result = ConfigurationUnitState::Unknown;

        if (database)
        {
            auto transaction = BeginTransaction("GetUnitState", database);
            result = database->GetUnitState(instanceIdentifier);
        }

        return result;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
#endif
    }

    IConfigurationUnitResultInformation ConfigurationDatabase::GetUnitResultInformation(const guid& instanceIdentifier)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
        auto database = std::atomic_load(&m_database);
        decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>()) result;

        if (database)
        {
            auto transaction = BeginTransaction("GetUnitResultInformation", database);
            auto resultInformation = database->GetUnitResultInformation(instanceIdentifier);

            if (resultInformation)
            {
                result = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
                result->Initialize(
                    std::get<0>(resultInformation.value()),
                    ConvertToUTF16(std::get<1>(resultInformation.value())),
                    ConvertToUTF16(std::get<2>(resultInformation.value())),
                    std::get<3>(resultInformation.value()));
            }
        }

        IConfigurationUnitResultInformation actualResult;
        if (result)
        {
            actualResult = *result;
        }

        return actualResult;
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();

        return {};
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
