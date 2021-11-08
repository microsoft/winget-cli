// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"
#include <winget/Manifest.h>

using namespace AppInstaller;

namespace AppInstaller::Repository::Microsoft::Schema::V1_4
{
    // Select a bunch of ids by values.
    static std::map<std::string, SQLite::rowid_t> SelectIdsByValues(const SQLite::Connection& connection, std::string_view tableName, std::string_view columnName, const std::vector<Utility::NormalizedString>& values);

    struct DependenciesTableColumnInfo
    {
        std::string_view Name;
    };

    struct DependenciesTable
    {
        // Get the table name.
        static std::string_view TableName();

        // Creates the table with named indices.
        static void Create(SQLite::Connection& connection);

        // Add the dependencies for the specific manifest.
        static void AddDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId);

        // update the dependencies for the specific manifest.
        static bool UpdateDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId);

        // Remove the dependencies by manifest id
        static void RemoveDependencies(SQLite::Connection& connection, SQLite::rowid_t manifestRowId);

        // Get dependencies the dependencies
        static std::map<Manifest::Dependency, SQLite::rowid_t> GetDependenciesByManifestRowId(SQLite::Connection& connection, SQLite::rowid_t manifestRowId);

        // Get the dependencies
        static bool ValidateDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest);

        static bool VerifyDependenciesStructureForManifestDelete(SQLite::Connection& connection, const Manifest::Manifest& manifest);

        static void PrepareForPackaging(SQLite::Connection& connection);
    };
}