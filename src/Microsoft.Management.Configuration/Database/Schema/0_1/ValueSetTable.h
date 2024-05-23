// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include <winget/SQLiteWrapper.h>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    struct ValueSetTable
    {
        ValueSetTable(AppInstaller::SQLite::Connection& connection);

        // Creates the set info table.
        void Create();

        // Adds the given value set to the table.
        // Returns the row id of the added set.
        AppInstaller::SQLite::rowid_t Add(const winrt::Windows::Foundation::Collections::ValueSet& valueSet);

        // Gets the value set from the table.
        winrt::Windows::Foundation::Collections::ValueSet GetValueSet(AppInstaller::SQLite::rowid_t rootRowId);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
