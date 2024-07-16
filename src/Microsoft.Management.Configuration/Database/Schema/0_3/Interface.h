// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/Schema/IConfigurationDatabase.h"
#include "Database/Schema/0_2/Interface.h"

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_3
{
    struct Interface : public V0_2::Interface
    {
        using V0_2::Interface::Interface;

        const AppInstaller::SQLite::Version& GetSchemaVersion() override;

        // Version 0.1
        void InitializeDatabase() override;
        void RemoveSet(AppInstaller::SQLite::rowid_t target) override;

        // Version 0.2
        bool MigrateFrom(IConfigurationDatabase* current) override;
        void AddQueueItem(const GUID& instanceIdentifier, const std::string& objectName) override;
        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> GetQueueItems() override;

        // Version 0.3
        std::vector<StatusItemTuple> GetStatusSince(int64_t changeIdentifier) override;
        std::tuple<int64_t, std::vector<StatusItemTuple>> GetStatusBaseline() override;
        void AddListener(const std::string& objectName) override;
        void RemoveListener(const std::string& objectName) override;
        std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, DWORD>> GetChangeListeners() override;
        void UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state) override;
        void UpdateSetInQueue(const guid& setInstanceIdentifier, bool inQueue) override;
        void UpdateUnitState(const guid& setInstanceIdentifier, const ConfigurationSetChangeDataPtr& changeData) override;
        ConfigurationSetState GetSetState(const guid& instanceIdentifier) override;
        std::chrono::system_clock::time_point GetSetFirstApply(const guid& instanceIdentifier) override;
        std::chrono::system_clock::time_point GetSetApplyBegun(const guid& instanceIdentifier) override;
        std::chrono::system_clock::time_point GetSetApplyEnded(const guid& instanceIdentifier) override;
        ConfigurationUnitState GetUnitState(const guid& instanceIdentifier) override;
        std::optional<std::tuple<HRESULT, std::string, std::string, ConfigurationUnitResultSource>> GetUnitResultInformation(const guid& instanceIdentifier) override;

    private:
        // Unconditionally attempts to migrate from the 0.2 base.
        void MigrateFrom0_2();
    };
}
