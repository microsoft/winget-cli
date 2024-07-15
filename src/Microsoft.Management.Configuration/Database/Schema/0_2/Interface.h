// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/Schema/IConfigurationDatabase.h"
#include "Database/Schema/0_1/Interface.h"

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_2
{
    struct Interface : public V0_1::Interface
    {
        using V0_1::Interface::Interface;

        const AppInstaller::SQLite::Version& GetSchemaVersion() override;

        // Version 0.1
        void InitializeDatabase() override;

        // Version 0.2
        bool MigrateFrom(IConfigurationDatabase* current) override;
        void AddQueueItem(const GUID& instanceIdentifier, const std::string& objectName) override;
        void SetActiveQueueItem(const std::string& objectName) override;
        std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, DWORD, bool>> GetQueueItems() override;
        void RemoveQueueItem(const std::string& objectName) override;

    private:
        // Unconditionally attempts to migrate from the 0.1 base.
        void MigrateFrom0_1();
    };
}
