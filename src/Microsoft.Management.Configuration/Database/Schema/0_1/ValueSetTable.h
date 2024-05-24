// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Windows.Foundation.h"
#include "winrt/Microsoft.Management.Configuration.h"
#include <winget/SQLiteWrapper.h>
#include <optional>
#include <vector>

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

#define CreatePropertyTypeTraits(_prop_,_type_,_support_) \
        template <> \
        struct PropertyTypeTraits<Windows::Foundation::PropertyType::_prop_> \
        { \
            using value_t = _type_; \
            constexpr static bool SupportedByStatement = _support_; \
        }

        CreatePropertyTypeTraits(UInt8, uint8_t, true);
        CreatePropertyTypeTraits(Int16, int16_t, true);
        CreatePropertyTypeTraits(UInt16, uint16_t, true);
        CreatePropertyTypeTraits(Int32, int32_t, true);
        CreatePropertyTypeTraits(UInt32, uint32_t, true);
        CreatePropertyTypeTraits(Int64, int64_t, true);
        CreatePropertyTypeTraits(UInt64, uint64_t, true);
        CreatePropertyTypeTraits(Single, float, true);
        CreatePropertyTypeTraits(Double, double, true);
        CreatePropertyTypeTraits(Boolean, bool, true);
        CreatePropertyTypeTraits(Guid, GUID, true);

        CreatePropertyTypeTraits(Char16, char16_t, false);
        CreatePropertyTypeTraits(String, winrt::hstring, false);
        CreatePropertyTypeTraits(Inspectable, winrt::Windows::Foundation::Collections::ValueSet, false);
        CreatePropertyTypeTraits(DateTime, winrt::clock::time_point, false);
        CreatePropertyTypeTraits(TimeSpan, winrt::clock::duration, false);
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

        // Gets an array from the table for the given property type (the non-array type).
        template <Windows::Foundation::PropertyType PropertyType>
        std::optional<std::vector<typename details::PropertyTypeTraits<PropertyType>::value_t>> GetArray(AppInstaller::SQLite::rowid_t rootRowId)
        {
            using value_t = details::PropertyTypeTraits<PropertyType>::value_t;
            std::optional<std::vector<value_t>> result;
            std::optional<AppInstaller::SQLite::Statement> statement = GetArrayStatement(rootRowId);
            if (statement)
            {
                result = std::make_optional<std::vector<value_t>>();
                while (statement->Step())
                {
                    result->emplace_back(GetValue(statement.value(), 0));
                }
            }
            return result;
        }

    private:
        // Gets a value of the template type using the statement and column provided.
        // The property type is not verified.
        template <Windows::Foundation::PropertyType PropertyType>
        details::PropertyTypeTraits<PropertyType>::value_t GetValue(AppInstaller::SQLite::Statement& statement, int column)
        {
            using traits = details::PropertyTypeTraits<PropertyType>;
            if constexpr (traits::SupportedByStatement)
            {
                return statement.GetColumn<traits::value_t>(column);
            }
            else
            {
                traits::value_t result{};
                GetValue(statement, column, result);
                return result;
            }
        }

        // Gets a statement for selecting the items in an array in order.
        // Column 0 contains the value.
        std::optional<AppInstaller::SQLite::Statement> GetArrayStatement(AppInstaller::SQLite::rowid_t rootRowId);

        void GetValue(AppInstaller::SQLite::Statement& statement, int column, char16_t& value);
        void GetValue(AppInstaller::SQLite::Statement& statement, int column, winrt::hstring& value);
        void GetValue(AppInstaller::SQLite::Statement& statement, int column, winrt::Windows::Foundation::Collections::ValueSet& value);
        void GetValue(AppInstaller::SQLite::Statement& statement, int column, winrt::clock::time_point& value);
        void GetValue(AppInstaller::SQLite::Statement& statement, int column, winrt::clock::duration& value);

        AppInstaller::SQLite::Connection& m_connection;
    };
}
