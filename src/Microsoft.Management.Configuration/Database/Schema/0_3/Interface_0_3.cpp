// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "Database/Schema/0_1/SetInfoTable.h"
#include "Database/Schema/0_2/QueueTable.h"
#include "StatusItemTable.h"
#include "ChangeListenerTable.h"
#include <winget/SQLiteMetadataTable.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_3
{
    static constexpr AppInstaller::SQLite::Version s_InterfaceVersion{ 0, 3 };

    const AppInstaller::SQLite::Version& Interface::GetSchemaVersion()
    {
        return s_InterfaceVersion;
    }

    void Interface::InitializeDatabase()
    {
        V0_2::Interface::InitializeDatabase();

        Savepoint savepoint = Savepoint::Create(*m_storage, "InitializeDatabase_0_3");
        MigrateFrom0_2();
        savepoint.Commit();
    }

    void Interface::RemoveSet(AppInstaller::SQLite::rowid_t target)
    {
        Savepoint savepoint = Savepoint::Create(*m_storage, "RemoveSet_0_3");

        V0_2::Interface::RemoveSet(target);

        StatusItemTable statusItemTable(*m_storage);
        statusItemTable.RemoveForSet(target);

        savepoint.Commit();
    }

    bool Interface::MigrateFrom(IConfigurationDatabase* current)
    {
        auto currentSchemaVersion = current->GetSchemaVersion();
        if (currentSchemaVersion < s_InterfaceVersion)
        {
            if (V0_2::Interface::MigrateFrom(current))
            {
                Savepoint savepoint = Savepoint::Create(*m_storage, "MigrateFrom0_2");

                MigrateFrom0_2();
                s_InterfaceVersion.SetSchemaVersion(*m_storage);

                savepoint.Commit();

                return true;
            }
        }
        else if (currentSchemaVersion == s_InterfaceVersion)
        {
            return true;
        }

        return false;
    }

    void Interface::AddQueueItem(const GUID& instanceIdentifier, const std::string& objectName)
    {
        V0_2::QueueTable queueTable(*m_storage);
        queueTable.AddQueueItemWithProcess(instanceIdentifier, objectName);
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> Interface::GetQueueItems()
    {
        V0_2::QueueTable queueTable(*m_storage);
        return queueTable.GetQueueItemsWithProcess();
    }

    void Interface::MigrateFrom0_2()
    {
        V0_2::QueueTable queueTable(*m_storage);
        queueTable.AddProcessColumn();

        ChangeListenerTable changeListenerTable(*m_storage);
        changeListenerTable.Create();

        StatusItemTable statusItemTable(*m_storage);
        statusItemTable.Create();
    }

    std::vector<IConfigurationDatabase::StatusItemTuple> Interface::GetStatusSince(int64_t changeIdentifier)
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetStatusSince(changeIdentifier);
    }

    std::tuple<int64_t, std::vector<IConfigurationDatabase::StatusItemTuple>> Interface::GetStatusBaseline()
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetStatusBaseline();
    }

    void Interface::AddListener(const std::string& objectName)
    {
        ChangeListenerTable changeListenerTable(*m_storage);
        changeListenerTable.AddChangeListener(objectName);
    }

    void Interface::RemoveListener(const std::string& objectName)
    {
        ChangeListenerTable changeListenerTable(*m_storage);
        changeListenerTable.RemoveChangeListener(objectName);
    }

    std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, DWORD>> Interface::GetChangeListeners()
    {
        ChangeListenerTable changeListenerTable(*m_storage);
        return changeListenerTable.GetChangeListeners();
    }

    void Interface::UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state)
    {
        StatusItemTable statusItemTable(*m_storage);
        statusItemTable.UpdateSetState(setInstanceIdentifier, state);
    }

    void Interface::UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue)
    {
        StatusItemTable statusItemTable(*m_storage);
        statusItemTable.UpdateSetInQueue(setInstanceIdentifier, inQueue);
    }

    void Interface::UpdateUnitState(const guid& setInstanceIdentifier, const ConfigurationSetChangeDataPtr& changeData)
    {
        StatusItemTable statusItemTable(*m_storage);
        statusItemTable.UpdateUnitState(setInstanceIdentifier, changeData);
    }

    ConfigurationSetState Interface::GetSetState(const guid& instanceIdentifier)
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetSetState(instanceIdentifier);
    }

    std::chrono::system_clock::time_point Interface::GetSetFirstApply(const guid& instanceIdentifier)
    {
        V0_1::SetInfoTable setInfoTable(*m_storage);
        return setInfoTable.GetSetFirstApply(instanceIdentifier);
    }

    std::chrono::system_clock::time_point Interface::GetSetApplyBegun(const guid& instanceIdentifier)
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetSetApplyBegun(instanceIdentifier);
    }

    std::chrono::system_clock::time_point Interface::GetSetApplyEnded(const guid& instanceIdentifier)
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetSetApplyEnded(instanceIdentifier);
    }

    ConfigurationUnitState Interface::GetUnitState(const guid& instanceIdentifier)
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetUnitState(instanceIdentifier);
    }

    std::optional<std::tuple<HRESULT, std::string, std::string, ConfigurationUnitResultSource>> Interface::GetUnitResultInformation(const guid& instanceIdentifier)
    {
        StatusItemTable statusItemTable(*m_storage);
        return statusItemTable.GetUnitResultInformation(instanceIdentifier);
    }
}
