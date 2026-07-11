// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteStatementBuilder.h>
#include <winget/Manifest.h>

namespace AppInstaller::Repository::Microsoft::Schema::V1_4
{
    using namespace AppInstaller;

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

        // Drops the table.
        static void Drop(SQLite::Connection& connection);

        static bool Exists(const SQLite::Connection& connection);

        // Add the dependencies for the specific manifest.
        static void AddDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId);

        // update the dependencies for the specific manifest.
        static bool UpdateDependencies(SQLite::Connection& connection, const Manifest::Manifest& manifest, SQLite::rowid_t manifestRowId);

        // Remove the dependencies by manifest id
        static void RemoveDependencies(SQLite::Connection& connection, SQLite::rowid_t manifestRowId);

        // Get dependencies the dependencies
        static std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId);

        // Get dependencies by package id.
        static std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t packageId);

        // Check dependencies table consistency.
        static bool CheckConsistency(const SQLite::Connection& connection, bool log);

        // Checks if the row id is present in the column denoted by the value supplied.
        static bool IsValueReferenced(const SQLite::Connection& connection, std::string_view valueName, SQLite::rowid_t valueRowId);

        // The dependencies table and corresponding index are dropped.
        static void PrepareForPackaging(SQLite::Connection& connection);

        // Get all min version values of the given manifest's dependencies, used for VersionTable cleanup when updating or removing a manifest.
        static std::vector<SQLite::rowid_t> GetDependenciesMinVersionsRowIdByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestRowId);

        // Get all dependencies with min versions in the dependencies table, used during consistency check. Returning a list of <PackageRowId, VersionString> pair.
        static std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> GetAllDependenciesWithMinVersions(const SQLite::Connection& connection);
    };
}
