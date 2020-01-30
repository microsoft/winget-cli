// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"

#include <wil/result_macros.h>
#include <string_view>

namespace AppInstaller::Repository::Microsoft::Schema
{
    using namespace std::string_view_literals;
    // Named metadata values are defined here to reduce the risk of duplicate names.

    // Version 1.0
    static constexpr std::string_view s_MetadataValueName_MajorVersion = "majorVersion"sv;
    static constexpr std::string_view s_MetadataValueName_MinorVersion = "minorVersion"sv;
    static constexpr std::string_view s_MetadataValueName_LastWriteTime = "lastwritetime"sv;

    // The metadata table for the index.
    // Contains a fixed-schema set of named values that can be used to determine how to read the rest of the index.
    struct MetadataTable
    {
        static void Create(SQLite::Connection& connection);

        // Gets the named value from the metadata table, interpreting it as the given type.
        template <typename Value>
        static Value GetNamedValue(SQLite::Connection& connection, std::string_view name)
        {
            SQLite::Statement statement = GetNamedValueStatement(connection, name);
            return statement.GetColumn<Value>(0);
        }

        // Sets the named value into the metadata table.
        template <typename Value>
        static void SetNamedValue(SQLite::Connection& connection, std::string_view name, Value&& v)
        {
            SQLite::Statement statement = SetNamedValueStatement(connection, name);
            statement.Bind(2, std::forward<Value>(v));
            statement.Execute();
        }

    private:
        // Internal function that gets the named value.
        static SQLite::Statement GetNamedValueStatement(SQLite::Connection& connection, std::string_view name);

        // Internal function that sets the named value.
        static SQLite::Statement SetNamedValueStatement(SQLite::Connection& connection, std::string_view name);
    };
}
