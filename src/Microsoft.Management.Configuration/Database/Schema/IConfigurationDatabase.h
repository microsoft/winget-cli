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
        using ConfigurationSetPtr = decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>());
        using ConfigurationUnitPtr = decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnit>>());

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

        // Gets all of the sets in the database.
        virtual std::vector<ConfigurationSetPtr> GetSets() = 0;
    };
}
