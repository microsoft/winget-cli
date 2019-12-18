// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"

#include <wil/result_macros.h>

namespace AppInstaller::Repository::Microsoft::Schema
{
    // Named metadata values are defined here to reduce the risk of duplicate names.

    // Version 1.0
    extern char const* const s_MetadataValueName_MajorVersion;
    extern char const *const s_MetadataValueName_MinorVersion;

    // The metadata table for the index.
    // Contains a fixed-schema set of named values that can be used to determine how to read the rest of the index.
    struct MetadataTable
    {
        static void Create(SQLite::Connection& connection);

        // Gets the named value from the metadata table, interpreting it as the given type.
        template <typename Value>
        static Value GetNamedValue(SQLite::Connection& connection, const std::string& name)
        {
            SQLite::Statement statement = GetNamedValueStatement(connection, name);
            return statement.GetColumn<Value>(0);
        }

        // Sets the named value into the metadata table.
        template <typename Value>
        static void SetNamedValue(SQLite::Connection& connection, const std::string& name, Value&& v)
        {
            SQLite::Statement statement = SetNamedValueStatement(connection, name);
            statement.Bind(2, std::forward<Value>(v));
            THROW_HR_IF(E_UNEXPECTED, statement.Step());
        }

    private:
        // Internal function that gets the named value.
        SQLite::Statement GetNamedValueStatement(SQLite::Connection& connection, const std::string& name);

        // Internal function that sets the named value.
        SQLite::Statement SetNamedValueStatement(SQLite::Connection& connection, const std::string& name);
    };
}
