// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>
#include <vector>
#include <tuple>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_3
{
    struct ChangeListenerTable
    {
        ChangeListenerTable(AppInstaller::SQLite::Connection& connection);

        // Creates the table.
        void Create();

        // Adds a new change listener to the table.
        void AddChangeListener(const std::string& objectName);

        // Removes the change listener with the given name from the table.
        void RemoveChangeListener(const std::string& objectName);

        // Gets all change listeners.
        std::vector<std::tuple<std::string, std::chrono::system_clock::time_point, DWORD>> GetChangeListeners();

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
