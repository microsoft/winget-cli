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

    template <typename OperationT>
    auto ConfigurationDatabase::ExecuteReadOperation(std::string_view operationName, OperationT&& operation, bool requireDatabase) const
    {
        using ResultT = decltype(operation(std::declval<std::shared_ptr<IConfigurationDatabase>&>()));
        ResultT result{};

#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
            auto database = std::atomic_load(&m_database);

            if (database)
            {
                auto transaction = BeginTransaction(operationName, false, database);
                result = operation(database);
            }
            else if (requireDatabase)
            {
                THROW_HR(E_NOT_VALID_STATE);
            }
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif

        return result;
    }

    template <typename OperationT>
    void ConfigurationDatabase::ExecuteWriteOperation(std::string_view operationName, OperationT&& operation, bool silentlyIgnoreNoDatabase)
    {
#ifdef AICLI_DISABLE_TEST_HOOKS
        // While under development, treat errors escaping this function as a test hook.
        try
        {
#endif
            auto database = std::atomic_load(&m_database);

            if (!database)
            {
                THROW_HR_IF(E_NOT_VALID_STATE, !silentlyIgnoreNoDatabase);
                return;
            }

            auto transaction = BeginTransaction(operationName, true, database);
            operation(database);
            std::atomic_load(&m_connection)->SetLastWriteTime();
            transaction->Commit();
#ifdef AICLI_DISABLE_TEST_HOOKS
        }
        CATCH_LOG();
#endif
    }

    std::vector<ConfigurationDatabase::ConfigurationSetPtr> ConfigurationDatabase::GetSetHistory() const
    {
        return ExecuteReadOperation("GetSetHistory",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetSets();
            });
    }

    ConfigurationDatabase::ConfigurationSetPtr ConfigurationDatabase::GetSet(const GUID& instanceIdentifier) const
    {
        return ExecuteReadOperation("GetSet",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetSet(instanceIdentifier);
            });
    }

    void ConfigurationDatabase::WriteSetHistory(const Configuration::ConfigurationSet& configurationSet, bool preferNewHistory)
    {
        THROW_HR_IF_NULL(E_POINTER, configurationSet);

        ExecuteWriteOperation("WriteSetHistory",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
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
            });
    }

    void ConfigurationDatabase::RemoveSetHistory(const Configuration::ConfigurationSet& configurationSet)
    {
        THROW_HR_IF_NULL(E_POINTER, configurationSet);

        ExecuteWriteOperation("RemoveSetHistory",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
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
            }, true);
    }


    void ConfigurationDatabase::AddQueueItem(const Configuration::ConfigurationSet& configurationSet, const std::string& objectName)
    {
        THROW_HR_IF_NULL(E_POINTER, configurationSet);

        ExecuteWriteOperation("AddQueueItem",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->AddQueueItem(configurationSet.InstanceIdentifier(), objectName);
            });
    }

    void ConfigurationDatabase::SetActiveQueueItem(const std::string& objectName)
    {
        ExecuteWriteOperation("SetActiveQueueItem",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->SetActiveQueueItem(objectName);
            });
    }

    std::vector<ConfigurationDatabase::QueueItem> ConfigurationDatabase::GetQueueItems() const
    {
        return ExecuteReadOperation("GetQueueItems",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                std::vector<QueueItem> result;
                auto queueItems = database->GetQueueItems();
                result.reserve(queueItems.size());

                for (const auto& item : queueItems)
                {
                    QueueItem resultItem;
                    std::tie(resultItem.SetInstanceIdentifier, resultItem.ObjectName, resultItem.QueuedAt, resultItem.ProcessId, resultItem.Active) = item;
                    result.emplace_back(std::move(resultItem));
                }

                return result;
            }, true);
    }

    void ConfigurationDatabase::RemoveQueueItem(const std::string& objectName)
    {
        ExecuteWriteOperation("RemoveQueueItem",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->RemoveQueueItem(objectName);
            });
    }

    std::vector<ConfigurationDatabase::StatusItem> ConfigurationDatabase::GetStatusSince(int64_t changeIdentifier) const
    {
        return ExecuteReadOperation("GetStatusSince",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return ConvertStatusItems(database->GetStatusSince(changeIdentifier));
            });
    }

    ConfigurationDatabase::StatusBaseline ConfigurationDatabase::GetStatusBaseline() const
    {
        return ExecuteReadOperation("GetStatusBaseline",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                auto [changeIdentifier, setStatus] = database->GetStatusBaseline();

                StatusBaseline result{};
                result.ChangeIdentifier = changeIdentifier;
                result.SetStatus = ConvertStatusItems(setStatus);
                return result;
            });
    }

    void ConfigurationDatabase::AddListener(const std::string& objectName)
    {
        ExecuteWriteOperation("AddListener",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->AddListener(objectName);
            });
    }

    void ConfigurationDatabase::RemoveListener(const std::string& objectName)
    {
        ExecuteWriteOperation("RemoveListener",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->RemoveListener(objectName);
            });
    }

    std::vector<ConfigurationDatabase::StatusChangeListener> ConfigurationDatabase::GetChangeListeners() const
    {
        return ExecuteReadOperation("GetChangeListeners",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                std::vector<StatusChangeListener> result;

                for (const auto& item : database->GetChangeListeners())
                {
                    StatusChangeListener listener{};
                    std::tie(listener.ObjectName, listener.Started, listener.ProcessId) = item;
                    result.emplace_back(std::move(listener));
                }

                return result;
            });
    }

    void ConfigurationDatabase::UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state)
    {
        ExecuteWriteOperation("UpdateSetState",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->UpdateSetState(setInstanceIdentifier, state);
            });
    }

    void ConfigurationDatabase::UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue)
    {
        ExecuteWriteOperation("UpdateSetInQueue",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->UpdateSetInQueue(setInstanceIdentifier, inQueue);
            });
    }

    void ConfigurationDatabase::UpdateUnitState(const guid& setInstanceIdentifier, const com_ptr<implementation::ConfigurationSetChangeData>& changeData)
    {
        ExecuteWriteOperation("UpdateUnitState",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                database->UpdateUnitState(setInstanceIdentifier, changeData);
            });
    }

    ConfigurationSetState ConfigurationDatabase::GetSetState(const guid& instanceIdentifier)
    {
        return ExecuteReadOperation("GetSetState",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetSetState(instanceIdentifier);
            });
    }

    std::chrono::system_clock::time_point ConfigurationDatabase::GetSetFirstApply(const guid& instanceIdentifier)
    {
        return ExecuteReadOperation("GetSetFirstApply",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetSetFirstApply(instanceIdentifier);
            });
    }

    std::chrono::system_clock::time_point ConfigurationDatabase::GetSetApplyBegun(const guid& instanceIdentifier)
    {
        return ExecuteReadOperation("GetSetApplyBegun",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetSetApplyBegun(instanceIdentifier);
            });
    }

    std::chrono::system_clock::time_point ConfigurationDatabase::GetSetApplyEnded(const guid& instanceIdentifier)
    {
        return ExecuteReadOperation("GetSetApplyEnded",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetSetApplyEnded(instanceIdentifier);
            });
    }

    ConfigurationUnitState ConfigurationDatabase::GetUnitState(const guid& instanceIdentifier)
    {
        return ExecuteReadOperation("GetUnitState",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                return database->GetUnitState(instanceIdentifier);
            });
    }

    IConfigurationUnitResultInformation ConfigurationDatabase::GetUnitResultInformation(const guid& instanceIdentifier)
    {
        return ExecuteReadOperation("GetUnitResultInformation",
            [&](std::shared_ptr<IConfigurationDatabase>& database)
            {
                com_ptr<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>> result;

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

                IConfigurationUnitResultInformation actualResult;
                if (result)
                {
                    actualResult = *result;
                }

                return actualResult;
            });
    }

    ConfigurationDatabase::TransactionLock ConfigurationDatabase::BeginTransaction(std::string_view name, bool forWrite, std::shared_ptr<IConfigurationDatabase>& database) const
    {
        auto connection = std::atomic_load(&m_connection);
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, connection);

        TransactionLock result = connection->TryBeginTransaction(name, forWrite);

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

            result = connection->TryBeginTransaction(name, forWrite);
        }

        return result;
    }
}
