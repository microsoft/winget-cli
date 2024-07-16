// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "QueueTable.h"
#include <winget/SQLiteMetadataTable.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_2
{
    static constexpr AppInstaller::SQLite::Version s_InterfaceVersion{ 0, 2 };

    const AppInstaller::SQLite::Version& Interface::GetSchemaVersion()
    {
        return s_InterfaceVersion;
    }

    void Interface::InitializeDatabase()
    {
        V0_1::Interface::InitializeDatabase();

        Savepoint savepoint = Savepoint::Create(*m_storage, "InitializeDatabase_0_2");
        MigrateFrom0_1();
        savepoint.Commit();
    }

    bool Interface::MigrateFrom(IConfigurationDatabase* current)
    {
        auto currentSchemaVersion = current->GetSchemaVersion();
        if (currentSchemaVersion < s_InterfaceVersion)
        {
            if (V0_1::Interface::MigrateFrom(current))
            {
                Savepoint savepoint = Savepoint::Create(*m_storage, "MigrateFrom0_1");

                MigrateFrom0_1();
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
        QueueTable queueTable(*m_storage);
        queueTable.AddQueueItemWithoutProcess(instanceIdentifier, objectName);
    }

    void Interface::SetActiveQueueItem(const std::string& objectName)
    {
        QueueTable queueTable(*m_storage);
        queueTable.SetActiveQueueItem(objectName);
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> Interface::GetQueueItems()
    {
        QueueTable queueTable(*m_storage);
        return queueTable.GetQueueItemsWithoutProcess();
    }

    void Interface::RemoveQueueItem(const std::string& objectName)
    {
        QueueTable queueTable(*m_storage);
        queueTable.RemoveQueueItem(objectName);
    }

    void Interface::MigrateFrom0_1()
    {
        QueueTable queueTable(*m_storage);
        queueTable.Create();
    }
}
