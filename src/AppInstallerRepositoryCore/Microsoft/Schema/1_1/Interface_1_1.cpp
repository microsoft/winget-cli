// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_1/Interface.h"

#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/NameTable.h"
#include "Microsoft/Schema/1_0/MonikerTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/ChannelTable.h"

#include "Microsoft/Schema/1_0/PathPartTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"

#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_0/CommandsTable.h"
#include "Microsoft/Schema/1_1/PackageFamilyNameTable.h"
#include "Microsoft/Schema/1_1/ProductCodeTable.h"

#include "Microsoft/Schema/1_1/SearchResultsTable.h"

#include "Microsoft/Schema/1_1/ManifestMetadataTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_1
{
    namespace
    {
        std::vector<Utility::NormalizedString> GetSystemReferenceStrings(
            const Manifest::Manifest& manifest,
            std::function<const Utility::NormalizedString&(const Manifest::ManifestInstaller&)> func)
        {
            std::set<Utility::NormalizedString> set;

            for (const auto& installer : manifest.Installers)
            {
                const Utility::NormalizedString& string = func(installer);
                if (!string.empty())
                {
                    set.emplace(Utility::FoldCase(string));
                }
            }

            std::vector<Utility::NormalizedString> result;
            for (auto&& string : set)
            {
                result.emplace_back(string);
            }

            return result;
        }

        std::vector<Utility::NormalizedString> GetPackageFamilyNames(const Manifest::Manifest& manifest)
        {
            return GetSystemReferenceStrings(manifest, [](const Manifest::ManifestInstaller& i) -> const Utility::NormalizedString& { return i.PackageFamilyName; });
        }

        std::vector<Utility::NormalizedString> GetProductCodes(const Manifest::Manifest& manifest)
        {
            return GetSystemReferenceStrings(manifest, [](const Manifest::ManifestInstaller& i) -> const Utility::NormalizedString& { return i.ProductCode; });
        }
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 1 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_1");

        V1_0::IdTable::Create(connection);
        V1_0::NameTable::Create(connection);
        V1_0::MonikerTable::Create(connection);
        V1_0::VersionTable::Create(connection);
        V1_0::ChannelTable::Create(connection);

        V1_0::PathPartTable::Create(connection);

        V1_0::ManifestTable::Create(connection, {
            { V1_0::IdTable::ValueName(), true, false },
            { V1_0::NameTable::ValueName(), false, false },
            { V1_0::MonikerTable::ValueName(), false, false },
            { V1_0::VersionTable::ValueName(), true, false },
            { V1_0::ChannelTable::ValueName(), true, false },
            { V1_0::PathPartTable::ValueName(), false, true }
            });

        V1_0::TagsTable::Create(connection);
        V1_0::CommandsTable::Create(connection);
        PackageFamilyNameTable::Create(connection);
        ProductCodeTable::Create(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_1");

        SQLite::rowid_t manifestId = V1_0::Interface::AddManifest(connection, manifest, relativePath);

        // Add the new 1.1 data
        // These system reference strings are all stored with their cases folded so that they can be
        // looked up ordinally; enabling the index to provide efficient searches.
        PackageFamilyNameTable::EnsureExistsAndInsert(connection, GetPackageFamilyNames(manifest), manifestId);
        ProductCodeTable::EnsureExistsAndInsert(connection, GetProductCodes(manifest), manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_1");

        auto [indexModified, manifestId] = V1_0::Interface::UpdateManifest(connection, manifest, relativePath);

        // Update new 1:N tables as necessary
        indexModified = PackageFamilyNameTable::UpdateIfNeededByManifestId(connection, GetPackageFamilyNames(manifest), manifestId) || indexModified;
        indexModified = ProductCodeTable::UpdateIfNeededByManifestId(connection, GetProductCodes(manifest), manifestId) || indexModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_1");

        SQLite::rowid_t manifestId = V1_0::Interface::RemoveManifest(connection, manifest, relativePath);

        // Remove all of the new 1:N data that is no longer referenced.
        PackageFamilyNameTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        ProductCodeTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        if (ManifestMetadataTable::Exists(connection))
        {
            ManifestMetadataTable::DeleteByManifestId(connection, manifestId);
        }

        savepoint.Commit();

        return manifestId;
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection)
    {
        PrepareForPackaging(connection, true);
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_0::Interface::CheckConsistency(connection, log);

        // If the v1.0 index was consistent, or if full logging of inconsistency was requested, check the v1.1 data.
        if (result || log)
        {
            result = PackageFamilyNameTable::CheckConsistency(connection, log) && result;
        }

        if (result || log)
        {
            result = ProductCodeTable::CheckConsistency(connection, log) && result;
        }

        return result;
    }

    ISQLiteIndex::SearchResult Interface::Search(const SQLite::Connection& connection, const SearchRequest& request) const
    {
        SearchRequest updatedRequest = request;
        return SearchInternal(connection, updatedRequest);
    }

    std::vector<std::string> Interface::GetMultiPropertyByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMultiProperty property) const
    {
        switch (property)
        {
        case PackageVersionMultiProperty::PackageFamilyName:
            return PackageFamilyNameTable::GetValuesByManifestId(connection, manifestId);
        case PackageVersionMultiProperty::ProductCode:
            return ProductCodeTable::GetValuesByManifestId(connection, manifestId);
        default:
            return V1_0::Interface::GetMultiPropertyByManifestId(connection, manifestId, property);
        }
    }

    ISQLiteIndex::MetadataResult Interface::GetMetadataByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId) const
    {
        ISQLiteIndex::MetadataResult result;

        if (ManifestMetadataTable::Exists(connection))
        {
            result = ManifestMetadataTable::GetMetadataByManifestId(connection, manifestId);
        }

        return result;
    }

    void Interface::SetMetadataByManifestId(SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionMetadata metadata, std::string_view value)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "setmetadatabymanifestid_v1_1");

        if (!ManifestMetadataTable::Exists(connection))
        {
            ManifestMetadataTable::Create(connection);
        }

        ManifestMetadataTable::SetMetadataByManifestId(connection, manifestId, metadata, value);

        savepoint.Commit();
    }

    std::unique_ptr<V1_0::SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<SearchResultsTable>(connection);
    }

    void Interface::PerformQuerySearch(V1_0::SearchResultsTable& resultsTable, const RequestMatch& query) const
    {
        // First, do an exact match search for the folded system reference strings
        // We do this first because it is exact, and likely won't match anything else if it matches this.
        PackageMatchFilter filter(PackageMatchField::PackageFamilyName, MatchType::Exact, Utility::FoldCase(query.Value));
        resultsTable.SearchOnField(filter);

        filter.Field = PackageMatchField::ProductCode;
        resultsTable.SearchOnField(filter);

        // Then do the 1.0 search
        V1_0::Interface::PerformQuerySearch(resultsTable, query);
    }

    ISQLiteIndex::SearchResult Interface::SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const
    {
        // Update any system reference strings to be folded
        auto foldIfNeeded = [](PackageMatchFilter& filter)
        {
            if ((filter.Field == PackageMatchField::PackageFamilyName || filter.Field == PackageMatchField::ProductCode) &&
                filter.Type == MatchType::Exact)
            {
                filter.Value = Utility::FoldCase(filter.Value);
            }
        };

        for (auto& inclusion : request.Inclusions)
        {
            foldIfNeeded(inclusion);
        }

        for (auto& filter : request.Filters)
        {
            foldIfNeeded(filter);
        }

        return V1_0::Interface::Search(connection, request);
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_1");

        V1_0::IdTable::PrepareForPackaging(connection);
        V1_0::NameTable::PrepareForPackaging(connection);
        V1_0::MonikerTable::PrepareForPackaging(connection);
        V1_0::VersionTable::PrepareForPackaging(connection);
        V1_0::ChannelTable::PrepareForPackaging(connection);

        V1_0::PathPartTable::PrepareForPackaging(connection);

        V1_0::ManifestTable::PrepareForPackaging(connection, {
            V1_0::VersionTable::ValueName(),
            V1_0::ChannelTable::ValueName(),
            V1_0::PathPartTable::ValueName(),
            });

        V1_0::TagsTable::PrepareForPackaging(connection, false);
        V1_0::CommandsTable::PrepareForPackaging(connection, false);
        PackageFamilyNameTable::PrepareForPackaging(connection, true, true);
        ProductCodeTable::PrepareForPackaging(connection, true, true);

        savepoint.Commit();

        if (vacuum)
        {
            // Force the database to actually shrink the file size.
            // This *must* be done outside of an active transaction.
            SQLite::Builder::StatementBuilder builder;
            builder.Vacuum();
            builder.Execute(connection);
        }
    }
}
