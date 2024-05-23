// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Database/Schema/IConfigurationDatabase.h"

#include "Database/Schema/0_1/Interface.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    AppInstaller::SQLite::Version IConfigurationDatabase::GetLatestVersion()
    {
        return { 0, 1 };
    }

    std::unique_ptr<IConfigurationDatabase> IConfigurationDatabase::CreateFor(std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage> storage)
    {
        using StorageT = std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage>;
        const AppInstaller::SQLite::Version& version = storage->GetVersion();

        if (version.MajorVersion == 0 ||
            version.IsLatest())
        {
            constexpr std::array<std::unique_ptr<IConfigurationDatabase>(*)(StorageT&& s), 1> versionCreatorMap =
            {
                [](StorageT&& s) { return std::unique_ptr<IConfigurationDatabase>(std::make_unique<Database::Schema::V0_1::Interface>(std::move(s))); },
            };

            size_t minorVersion = static_cast<size_t>(version.MinorVersion);
            if (minorVersion < versionCreatorMap.size())
            {
                return versionCreatorMap[minorVersion](std::move(storage));
            }
        }

        // We do not have the capacity to operate on this schema version
        THROW_WIN32(ERROR_NOT_SUPPORTED);
    }
}
