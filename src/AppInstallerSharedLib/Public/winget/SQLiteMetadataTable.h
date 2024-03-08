// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>

#include <wil/result_macros.h>
#include <string_view>

namespace AppInstaller::SQLite
{
    using namespace std::string_view_literals;

    static constexpr std::string_view s_MetadataValueName_DatabaseIdentifier = "databaseIdentifier"sv;
    static constexpr std::string_view s_MetadataValueName_MajorVersion = "majorVersion"sv;
    static constexpr std::string_view s_MetadataValueName_MinorVersion = "minorVersion"sv;
    static constexpr std::string_view s_MetadataValueName_LastWriteTime = "lastwritetime"sv;

    // The metadata table for the database.
    // Contains a fixed-schema set of named values that can be used to determine how to read the rest of the database.
    struct MetadataTable
    {
        static void Create(Connection& connection);

        // Gets the named value from the metadata table, interpreting it as the given type.
        template <typename Value>
        static Value GetNamedValue(const Connection& connection, std::string_view name)
        {
            Statement statement = GetNamedValueStatement(connection, name);
            return statement.GetColumn<Value>(0);
        }

        // Gets the named value from the metadata table, interpreting it as the given type.
        // Returns nullopt if the value is not present.
        template <typename Value>
        static std::optional<Value> TryGetNamedValue(const Connection& connection, std::string_view name)
        {
            std::optional<Statement> statement = TryGetNamedValueStatement(connection, name);
            if (statement)
            {
                return statement->GetColumn<Value>(0);
            }
            else
            {
                return std::nullopt;
            }
        }

        // Sets the named value into the metadata table.
        template <typename Value>
        static void SetNamedValue(const Connection& connection, std::string_view name, Value&& v)
        {
            Statement statement = SetNamedValueStatement(connection, name);
            statement.Bind(2, std::forward<Value>(v));
            statement.Execute();
        }

    private:
        // Internal function that gets the named value.
        static Statement GetNamedValueStatement(const Connection& connection, std::string_view name);

        // Internal function that gets the named value, or nullopt if it is not present.
        static std::optional<Statement> TryGetNamedValueStatement(const Connection& connection, std::string_view name);

        // Internal function that sets the named value.
        static Statement SetNamedValueStatement(const Connection& connection, std::string_view name);
    };
}
