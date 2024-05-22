// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Database/ConfigurationDatabase.h"
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
            std::filesystem::path databaseDirectoryName = AppInstaller::Filesystem::GetPathTo(PathName::LocalState) / anon::s_Database_DirectoryName;
            std::filesystem::path databaseFileName = databaseDirectoryName / anon::s_Database_FileName;

            {
                // TODO: Create and acquire named mutex for database
                if (!std::filesystem::is_regular_file(databaseFileName))
                {
                    if (std::filesystem::exists(databaseFileName))
                    {
                        std::filesystem::remove_all(databaseDirectoryName);
                    }


                }
            }
        }
    }

    // Gets all of the configuration sets from the database.
    std::vector<ConfigurationDatabase::ConfigurationSetPtr> ConfigurationDatabase::GetSetHistory() const
    {
        THROW_HR(E_NOTIMPL);
    }

    // Writes the given set to the database history, attempting to merge with a matching set if one exists unless forceNewHistory is true.
    void ConfigurationDatabase::WriteSetHistory(const Configuration::ConfigurationSet& configurationSet, bool forceNewHistory)
    {
        THROW_HR(E_NOTIMPL);
    }
}
