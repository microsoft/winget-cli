// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Database/Schema/IConfigurationDatabase.h"

#include "Database/Schema/0_1/Interface.h"
#include "Database/Schema/0_2/Interface.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        std::unique_ptr<IConfigurationDatabase> CreateForVersion(const AppInstaller::SQLite::Version& version, const std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage>& storage)
        {
            using StorageT = std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage>;

            if (version.MajorVersion == 0)
            {
                constexpr std::array<std::unique_ptr<IConfigurationDatabase>(*)(const StorageT& s), 2> versionCreatorMap =
                {
                    [](const StorageT& s) { return std::unique_ptr<IConfigurationDatabase>(std::make_unique<Database::Schema::V0_1::Interface>(s)); },
                    [](const StorageT& s) { return std::unique_ptr<IConfigurationDatabase>(std::make_unique<Database::Schema::V0_2::Interface>(s)); },
                };

                size_t minorVersion = static_cast<size_t>(version.MinorVersion);
                if (minorVersion >= 1 && minorVersion <= versionCreatorMap.size())
                {
                    return versionCreatorMap[minorVersion - 1](storage);
                }
            }

            // We do not have the capacity to operate on this schema version
            THROW_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    AppInstaller::SQLite::Version IConfigurationDatabase::GetLatestVersion()
    {
        return { 0, 2 };
    }

    std::unique_ptr<IConfigurationDatabase> IConfigurationDatabase::CreateFor(const std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage>& storage, bool allowMigration)
    {
        using StorageT = std::shared_ptr<AppInstaller::SQLite::SQLiteDynamicStorage>;
        const AppInstaller::SQLite::Version& version = storage->GetVersion();

        std::unique_ptr<IConfigurationDatabase> result = CreateForVersion(version, storage);

        AppInstaller::SQLite::Version latestVersion = GetLatestVersion();
        if (allowMigration && version < latestVersion)
        {
            // Always migrate to the latest version until a reason comes along to not do that
            std::unique_ptr<IConfigurationDatabase> latest = CreateForVersion(latestVersion, storage);
            THROW_WIN32_IF(ERROR_NOT_SUPPORTED, !latest->MigrateFrom(result.get()));
            result = std::move(latest);
        }

        return result;
    }

    void IConfigurationDatabase::AddQueueItem(const GUID&, const std::string&)
    {
    }

    void IConfigurationDatabase::SetActiveQueueItem(const std::string&)
    {
    }

    std::vector<std::tuple<GUID, std::string, std::chrono::system_clock::time_point, bool>> IConfigurationDatabase::GetQueueItems()
    {
        return {};
    }

    void IConfigurationDatabase::RemoveQueueItem(const std::string&)
    {
    }
}
