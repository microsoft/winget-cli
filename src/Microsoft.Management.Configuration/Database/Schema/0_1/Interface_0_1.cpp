// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Interface.h"
#include "SetInfoTable.h"
#include "UnitInfoTable.h"
#include "ConfigurationSetParser.h"
#include <AppInstallerDateTime.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::SQLite;
using namespace AppInstaller::Utility;

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    Interface::Interface(std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> storage) :
        m_storage(std::move(storage))
    {}

    void Interface::InitializeDatabase()
    {
        Savepoint savepoint = Savepoint::Create(*m_storage, "InitializeDatabase_0_1");

        Statement setJournalMode = Statement::Create(*m_storage, "PRAGMA journal_mode=WAL");
        setJournalMode.Execute();

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
        rowid_t setRowId = setInfoTable.Add(configurationSet);

        UnitInfoTable unitInfoTable(*m_storage);

        auto winrtUnits = configurationSet.Units();
        std::vector<ConfigurationUnit> units{ winrtUnits.Size() };
        winrtUnits.GetMany(0, units);

        for (const ConfigurationUnit& unit : units)
        {
            unitInfoTable.Add(unit, setRowId);
        }

        savepoint.Commit();
    }

    std::vector<IConfigurationDatabase::ConfigurationSetPtr> Interface::GetSets()
    {
        std::vector<IConfigurationDatabase::ConfigurationSetPtr> result;

        SetInfoTable setInfoTable(*m_storage);
        UnitInfoTable unitInfoTable(*m_storage);

        Statement getAllSets = setInfoTable.GetAllSetsStatement();
        while (getAllSets.Step())
        {
            using Columns = SetInfoTable::GetAllSetsStatementColumns;
            auto configurationSet = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>(getAllSets.GetColumn<GUID>(Columns::InstanceIdentifier));

            configurationSet->Name(hstring{ ConvertToUTF16(getAllSets.GetColumn<std::string>(Columns::Name)) });
            configurationSet->Origin(hstring{ ConvertToUTF16(getAllSets.GetColumn<std::string>(Columns::Origin)) });
            configurationSet->Path(hstring{ ConvertToUTF16(getAllSets.GetColumn<std::string>(Columns::Path)) });
            configurationSet->FirstApply(clock::from_sys(ConvertUnixEpochToSystemClock(getAllSets.GetColumn<int64_t>(Columns::FirstApply))));

            std::string schemaVersion = getAllSets.GetColumn<std::string>(Columns::SchemaVersion);
            configurationSet->SchemaVersion(hstring{ ConvertToUTF16(schemaVersion) });

            auto parser = ConfigurationSetParser::CreateForSchemaVersion(schemaVersion);
            configurationSet->Metadata(parser->ParseValueSet(getAllSets.GetColumn<std::string>(Columns::Metadata)));
            THROW_HR_IF(E_NOTIMPL, !getAllSets.GetColumn<std::string>(Columns::Parameters).empty());
            configurationSet->Variables(parser->ParseValueSet(getAllSets.GetColumn<std::string>(Columns::Variables)));

            std::vector<Configuration::ConfigurationUnit> winrtUnits;
            for (const auto& unit : unitInfoTable.GetAllUnitsForSet(getAllSets.GetColumn<rowid_t>(Columns::RowID)))
            {
                winrtUnits.emplace_back(*unit);
            }
            configurationSet->Units(std::move(winrtUnits));
        }

        return result;
    }
}
