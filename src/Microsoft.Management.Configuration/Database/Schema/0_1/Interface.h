// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/Schema/IConfigurationDatabase.h"

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    struct Interface : public IConfigurationDatabase
    {
        Interface(std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> storage);

        // Version 0.1
        void InitializeDatabase() override;
        void AddSet(const Configuration::ConfigurationSet& configurationSet) override;
        void UpdateSet(AppInstaller::SQLite::rowid_t target, const Configuration::ConfigurationSet& configurationSet) override;
        void RemoveSet(AppInstaller::SQLite::rowid_t target) override;
        std::vector<ConfigurationSetPtr> GetSets() override;
        std::optional<AppInstaller::SQLite::rowid_t> GetSetRowId(const GUID& instanceIdentifier) override;

    private:
        std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> m_storage;
    };
}
