// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "SetInfoTable.h"
#include "UnitInfoTable.h"

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    static constexpr AppInstaller::SQLite::Version s_InterfaceVersion{ 0, 1 };

    Interface::Interface(std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> storage) :
        m_storage(std::move(storage))
    {}

    const AppInstaller::SQLite::Version& Interface::GetSchemaVersion()
    {
        return s_InterfaceVersion;
    }

    void Interface::InitializeDatabase()
    {
        // Must enable WAL mode outside of a transaction
        THROW_HR_IF(E_UNEXPECTED, !m_storage->GetConnection().SetJournalMode("WAL"));

        Savepoint savepoint = Savepoint::Create(*m_storage, "InitializeDatabase_0_1");

        SetInfoTable setInfoTable(*m_storage);
        setInfoTable.Create();

        UnitInfoTable unitInfoTable(*m_storage);
        unitInfoTable.Create();

        savepoint.Commit();
    }

    void Interface::AddSet(const Configuration::ConfigurationSet& configurationSet)
    {
        Savepoint savepoint = Savepoint::Create(*m_storage, "AddSet_0_1");

        SetInfoTable setInfoTable(*m_storage);
        setInfoTable.Add(configurationSet);

        savepoint.Commit();
    }

    void Interface::UpdateSet(rowid_t target, const Configuration::ConfigurationSet& configurationSet)
    {
        Savepoint savepoint = Savepoint::Create(*m_storage, "UpdateSet_0_1");

        SetInfoTable setInfoTable(*m_storage);
        setInfoTable.Update(target, configurationSet);

        savepoint.Commit();
    }

    void Interface::RemoveSet(rowid_t target)
    {
        Savepoint savepoint = Savepoint::Create(*m_storage, "RemoveSet_0_1");

        SetInfoTable setInfoTable(*m_storage);
        setInfoTable.Remove(target);

        savepoint.Commit();
    }

    std::vector<IConfigurationDatabase::ConfigurationSetPtr> Interface::GetSets()
    {
        SetInfoTable setInfoTable(*m_storage);
        return setInfoTable.GetAllSets();
    }

    std::optional<rowid_t> Interface::GetSetRowId(const GUID& instanceIdentifier)
    {
        SetInfoTable setInfoTable(*m_storage);
        return setInfoTable.GetSetRowId(instanceIdentifier);
    }

    bool Interface::MigrateFrom(IConfigurationDatabase* current)
    {
        return current->GetSchemaVersion() == s_InterfaceVersion;
    }

    Interface::ConfigurationSetPtr Interface::GetSet(const GUID& instanceIdentifier)
    {
        SetInfoTable setInfoTable(*m_storage);
        return setInfoTable.GetSet(instanceIdentifier);
    }
}
