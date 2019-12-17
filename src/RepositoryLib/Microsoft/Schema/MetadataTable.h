// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"


namespace AppInstaller::Repository::Microsoft::Schema
{
    // Named metadata values are defined here to reduce the risk of duplicate names.
    extern char const* const s_MetadataValueName_MajorVersion;
    extern char const *const s_MetadataValueName_MinorVersion;

    // The metadata table for the index.
    // Contains a fixed-schema set of named values that can be used to determine how to read the rest of the index.
    struct MetadataTable
    {
        static void Create(SQLite::Connection& connection);

        // Gets the named value from the metadata table, interpreting it as the given type.
        template <typename Value>
        static Value GetNamedValue(const std::string& name)
        {
            SQLite::Statement statement = GetNamedValueStatement(name);
            return statement.GetColumn<Value>(0);
        }

    private:
        // Internal function that gets the named value.
        SQLite::Statement GetNamedValueStatement(const std::string& name);
    };
}
