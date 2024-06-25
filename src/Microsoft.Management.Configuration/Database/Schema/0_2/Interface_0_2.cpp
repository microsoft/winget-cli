// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "QueueTable.h"

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
        MigrateFrom0_1();
    }

    bool Interface::MigrateFrom(IConfigurationDatabase* current)
    {
        auto currentSchemaVersion = current->GetSchemaVersion();
        if (currentSchemaVersion < s_InterfaceVersion)
        {
            if (V0_1::Interface::MigrateFrom(current))
            {
                MigrateFrom0_1();
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
        queueTable.AddQueueItem(instanceIdentifier, objectName);
    }

    void Interface::SetActiveQueueItem(const std::string& objectName)
    {
        QueueTable queueTable(*m_storage);
        queueTable.SetActiveQueueItem(objectName);
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, bool>> Interface::GetQueueItems()
    {
        QueueTable queueTable(*m_storage);
        return queueTable.GetQueueItems();
    }

    void Interface::RemoveQueueItem(const std::string& objectName)
    {
        QueueTable queueTable(*m_storage);
        queueTable.RemoveQueueItem(objectName);
    }

    void Interface::MigrateFrom0_1()
    {
        Savepoint savepoint = Savepoint::Create(*m_storage, "MigrateFrom0_1");

        QueueTable queueTable(*m_storage);
        queueTable.Create();

        savepoint.Commit();
    }
}
