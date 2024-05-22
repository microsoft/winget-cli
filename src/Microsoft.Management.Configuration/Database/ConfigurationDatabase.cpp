// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Database/ConfigurationDatabase.h"
#include "Database/Schema/IConfigurationDatabase.h"
#include <winget/Filesystem.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace anon
    {
        constexpr std::string_view s_Database_DirectoryName = "history"sv;
        constexpr std::string_view s_Database_FileName = "config.db"sv;
    }

    ConfigurationDatabase::ConfigurationDatabase() = default;

    ConfigurationDatabase::ConfigurationDatabase(ConfigurationDatabase&&) = default;
    ConfigurationDatabase& ConfigurationDatabase::operator=(ConfigurationDatabase&&) = default;

    ConfigurationDatabase::~ConfigurationDatabase() = default;

    // Ensures that the database connection is established and the schema interface is created appropriately.
    // If `createIfNeeded` is false, this function will not create the database if it does not exist.
    // If not connected, any read methods will return empty results and any write methods will throw.
    void ConfigurationDatabase::EnsureOpened(bool createIfNeeded)
    {
        if (!m_database)
        {
            std::filesystem::path databaseDirectory = AppInstaller::Filesystem::GetPathTo(PathName::LocalState) / anon::s_Database_DirectoryName;
            std::filesystem::path databaseFile = databaseDirectory / anon::s_Database_FileName;

            {
                // TODO: Create and acquire named mutex for database
                if (!std::filesystem::is_regular_file(databaseFile) && createIfNeeded)
                {
                    if (std::filesystem::exists(databaseFile))
                    {
                        std::filesystem::remove_all(databaseDirectory);
                    }

                    std::filesystem::create_directories(databaseDirectory);

                    m_connection = StorageBaseDerivedThing::Create(databaseFile, LATEST_VERSION);
                    m_database = IConfigurationDatabase::CreateFor(m_connection);
                    m_database->InitializeDatabase();
                }
            }

            if (!m_database && std::filesystem::is_regular_file(databaseFile))
            {
                m_connection = StorageBaseDerivedThing::Open(databaseFile, ReadWrite);
                m_database = IConfigurationDatabase::CreateFor(m_connection);
            }
        }
    }

    // Gets all of the configuration sets from the database.
    std::vector<ConfigurationDatabase::ConfigurationSetPtr> ConfigurationDatabase::GetSetHistory() const
    {
        if (!m_database)
        {
            return {};
        }

        THROW_HR(E_NOTIMPL);
    }

    // Writes the given set to the database history, attempting to merge with a matching set if one exists unless forceNewHistory is true.
    void ConfigurationDatabase::WriteSetHistory(const Configuration::ConfigurationSet& configurationSet, bool forceNewHistory)
    {
        THROW_HR_IF_NULL(E_POINTER, configurationSet);
        THROW_HR_IF_NULL(E_NOT_VALID_STATE, m_database);

        THROW_HR(E_NOTIMPL);
    }
}
