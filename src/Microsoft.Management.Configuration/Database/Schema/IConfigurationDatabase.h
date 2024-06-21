// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "ConfigurationSet.h"
#include "ConfigurationUnit.h"
#include <winget/SQLiteVersion.h>
#include <winget/SQLiteDynamicStorage.h>
#include <wil/cppwinrt_wrl.h>
#include <memory>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Interface for interacting with the configuration database.
    struct IConfigurationDatabase
    {
        using ConfigurationSetPtr = winrt::com_ptr<implementation::ConfigurationSet>;
        using ConfigurationUnitPtr = winrt::com_ptr<implementation::ConfigurationUnit>;

        virtual ~IConfigurationDatabase() = default;

        // Gets the latest schema version for the configuration database.
        static AppInstaller::SQLite::Version GetLatestVersion();

        // Creates the version appropriate database object for the given storage.
        static std::unique_ptr<IConfigurationDatabase> CreateFor(std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> storage);

        // Version 0.1

        // Acts on a database that has been created but contains no tables beyond metadata.
        virtual void InitializeDatabase() = 0;

        // Adds the given set to the database.
        virtual void AddSet(const Configuration::ConfigurationSet& configurationSet) = 0;

        // Updates the set with the given row id using the given set.
        virtual void UpdateSet(AppInstaller::SQLite::rowid_t target, const Configuration::ConfigurationSet& configurationSet) = 0;

        // Removes the set with the given row id from the database.
        virtual void RemoveSet(AppInstaller::SQLite::rowid_t target) = 0;

        // Gets all of the sets in the database.
        virtual std::vector<ConfigurationSetPtr> GetSets() = 0;

        // Gets the row id of the set with the given instance identifier, if present.
        virtual std::optional<AppInstaller::SQLite::rowid_t> GetSetRowId(const GUID& instanceIdentifier) = 0;
    };
}
