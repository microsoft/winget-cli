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
        static std::map<Manifest::Dependency, SQLite::rowid_t> GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId);

        // Get dependencies by package id.
        static std::vector<std::pair<SQLite::rowid_t, Utility::Version>> GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId);

        static void PrepareForPackaging(SQLite::Connection& connection);
    };
}