// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Windows.Foundation.h"
#include "winrt/Microsoft.Management.Configuration.h"
#include <winget/SQLiteWrapper.h>

namespace winrt::Microsoft::Management::Configuration::implementation::Database::Schema::V0_1
{
    namespace details
    {
        // Compiler message meta-programming.
        struct PropertyTypeIsNotKnownToPropertyTypeTraits;

        template <Windows::Foundation::PropertyType PropertyType>
        struct PropertyTypeTraits
        {
            using value_t = PropertyTypeIsNotKnownToPropertyTypeTraits;
        };

        template <>
        struct PropertyTypeTraits<Windows::Foundation::PropertyType::UInt8>
        {
            using value_t = uint8_t;
        };
    }

    struct ValueSetTable
    {
        ValueSetTable(AppInstaller::SQLite::Connection& connection);

        // Creates the set info table.
        void Create();

        // Adds the given value set to the table.
        // Returns the row id of the added set.
        AppInstaller::SQLite::rowid_t Add(const winrt::Windows::Foundation::Collections::ValueSet& valueSet);

        // Adds the given vector to the table.
        // Returns the row id of the added vector.
        AppInstaller::SQLite::rowid_t Add(const winrt::Windows::Foundation::Collections::IVector<winrt::hstring>& values);

        // Gets the value set from the table.
        winrt::Windows::Foundation::Collections::ValueSet GetValueSet(AppInstaller::SQLite::rowid_t rootRowId);

        // Gets the string vector from the table.
        std::vector<winrt::hstring> GetStringVector(AppInstaller::SQLite::rowid_t rootRowId);

    private:
        AppInstaller::SQLite::Connection& m_connection;
    };
}
