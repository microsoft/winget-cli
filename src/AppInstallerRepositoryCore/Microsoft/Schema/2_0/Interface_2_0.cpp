// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/SQLiteMetadataTable.h>
#include "Microsoft/Schema/2_0/Interface.h"

#include "Microsoft/Schema/2_0/PackagesTable.h"

#include "Microsoft/Schema/2_0/TagsTable.h"
#include "Microsoft/Schema/2_0/CommandsTable.h"

#include "Microsoft/Schema/2_0/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace anonymous
    {
    }

    Interface::Interface(Utility::NormalizationVersion normVersion) : m_normalizer(normVersion)
    {
    }

    SQLite::Version Interface::GetVersion() const
    {
        return { 2, 0 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        m_internalInterface = CreateISQLiteIndex(SQLite::Version::LatestForMajor(1));

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v2_0");

        // Store the internal schema version in the metadata
        SQLite::Version internalVersion = m_internalInterface->GetVersion();
        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_InternalMajorVersion, static_cast<int>(internalVersion.MajorVersion));
        SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_InternalMinorVersion, static_cast<int>(internalVersion.MinorVersion));

        // We only create the internal tables at this point, the actual 2.0 tables are created in PrepareForPackaging
        m_internalInterface->CreateTables(connection, options);

        savepoint.Commit();

        m_internalInterfaceChecked = true;
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        EnsureInternalInterface(connection, true);
        return m_internalInterface->AddManifest(connection, manifest, relativePath);
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        EnsureInternalInterface(connection, true);
        return m_internalInterface->UpdateManifest(connection, manifest, relativePath);
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest)
    {
        EnsureInternalInterface(connection, true);
        return m_internalInterface->RemoveManifest(connection, manifest);
    }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        EnsureInternalInterface(connection, true);
        m_internalInterface->RemoveManifestById(connection, manifestId);
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection)
    {
        EnsureInternalInterface(connection, true);
        PrepareForPackaging(connection, true);
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->CheckConsistency(connection, log);
        }

        bool result = true;

#define AICLI_CHECK_CONSISTENCY(_check_) \
        if (result || log) \
        { \
            result = _check_ && result; \
        }

        // Check the 1:N map tables for consistency
        AICLI_CHECK_CONSISTENCY(TagsTable::CheckConsistency(connection, log));
        AICLI_CHECK_CONSISTENCY(CommandsTable::CheckConsistency(connection, log));

        // TODO: Check the *5* new-style 1:N tables as well

#undef AICLI_CHECK_CONSISTENCY

        return result;
    }

    ISQLiteIndex::SearchResult Interface::Search(const SQLite::Connection& connection, const SearchRequest& request) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->Search(connection, request);
        }

        // TODO: Move to SearchInternal and bring in 1.1 - 1.7 search request updating

        if (request.IsForEverything())
        {
            std::vector<SQLite::rowid_t> ids = PackagesTable::GetAllRowIds(connection, request.MaximumResults);

            SearchResult result;
            for (SQLite::rowid_t id : ids)
            {
                result.Matches.emplace_back(std::make_pair(id, PackageMatchFilter(PackageMatchField::Id, MatchType::Wildcard)));
            }

            result.Truncated = (request.MaximumResults && PackagesTable::GetCount(connection) > request.MaximumResults);

            return result;
        }

        // First phase, create the search results table and populate it with the initial results.
        // If the Query is provided, we search across many fields and put results in together.
        // If Inclusions has fields, we add these to the data.
        // If neither is defined, we take the first filter and use it as the initial results search.
        std::unique_ptr<SearchResultsTable> resultsTable = CreateSearchResultsTable(connection);
        bool inclusionsAttempted = false;

        if (request.Query)
        {
            // Perform searches across multiple tables to populate the initial results.
            PerformQuerySearch(*resultsTable.get(), request.Query.value());

            inclusionsAttempted = true;
        }

        if (!request.Inclusions.empty())
        {
            for (auto include : request.Inclusions)
            {
                for (MatchType match : GetMatchTypeOrder(include.Type))
                {
                    include.Type = match;
                    resultsTable->SearchOnField(include);
                }
            }

            inclusionsAttempted = true;
        }

        size_t filterIndex = 0;
        if (!inclusionsAttempted)
        {
            THROW_HR_IF(E_UNEXPECTED, request.Filters.empty());

            // Perform search for just the field matching the first filter
            PackageMatchFilter filter = request.Filters[0];

            for (MatchType match : GetMatchTypeOrder(filter.Type))
            {
                filter.Type = match;
                resultsTable->SearchOnField(filter);
            }

            // Skip the filter as we already know everything matches
            filterIndex = 1;
        }

        // Remove any duplicate manifest entries
        resultsTable->RemoveDuplicateManifestRows();

        // Second phase, for remaining filters, flag matching search results, then remove unflagged values.
        for (size_t i = filterIndex; i < request.Filters.size(); ++i)
        {
            PackageMatchFilter filter = request.Filters[i];

            resultsTable->PrepareToFilter();

            for (MatchType match : GetMatchTypeOrder(filter.Type))
            {
                filter.Type = match;
                resultsTable->FilterOnField(filter);
            }

            resultsTable->CompleteFilter();
        }

        return resultsTable->GetSearchResults(request.MaximumResults);
    }

    std::optional<std::string> Interface::GetPropertyByManifestId(const SQLite::Connection& connection, SQLite::rowid_t manifestId, PackageVersionProperty property) const
    {
        switch (property)
        {
        case AppInstaller::Repository::PackageVersionProperty::Id:
            return ManifestTable::GetValueById<IdTable>(connection, manifestId);
        case AppInstaller::Repository::PackageVersionProperty::Name:
            return ManifestTable::GetValueById<NameTable>(connection, manifestId);
        case AppInstaller::Repository::PackageVersionProperty::Version:
            return ManifestTable::GetValueById<VersionTable>(connection, manifestId);
        case AppInstaller::Repository::PackageVersionProperty::Channel:
            return "";
            // TODO: Add newer properties
        default:
            return {};
        }
    }

    std::vector<std::string> Interface::GetMultiPropertyByManifestId(const SQLite::Connection&, SQLite::rowid_t, PackageVersionMultiProperty) const
    {
        // TODO: Implement newer multi-properties
        return {};
    }

    std::optional<SQLite::rowid_t> Interface::GetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetManifestIdByKey(connection, id, version, channel);
        }

        THROW_HR(E_NOT_VALID_STATE);
    }

    std::optional<SQLite::rowid_t> Interface::GetManifestIdByManifest(const SQLite::Connection& connection, const Manifest::Manifest& manifest) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetManifestIdByManifest(connection, manifest);
        }

        THROW_HR(E_NOT_VALID_STATE);
    }

    std::vector<ISQLiteIndex::VersionKey> Interface::GetVersionKeysById(const SQLite::Connection& connection, SQLite::rowid_t id) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetVersionKeysById(connection, id);
        }

        THROW_HR(E_NOT_VALID_STATE);
    }

    ISQLiteIndex::MetadataResult Interface::GetMetadataByManifestId(const SQLite::Connection&, SQLite::rowid_t) const
    {
        return {};
    }

    void Interface::SetMetadataByManifestId(SQLite::Connection&, SQLite::rowid_t, PackageVersionMetadata, std::string_view)
    {
    }

    Utility::NormalizedName Interface::NormalizeName(std::string_view name, std::string_view publisher) const
    {
        return m_normalizer.Normalize(name, publisher);
    }

    std::set<std::pair<SQLite::rowid_t, Utility::NormalizedString>> Interface::GetDependenciesByManifestRowId(const SQLite::Connection& connection, SQLite::rowid_t rowid) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetDependenciesByManifestRowId(connection, rowid);
        }

        THROW_HR(E_NOT_VALID_STATE);
    }

    std::vector<std::pair<SQLite::rowid_t, Utility::NormalizedString>> Interface::GetDependentsById(const SQLite::Connection& connection, AppInstaller::Manifest::string_t id) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetDependentsById(connection, id);
        }

        THROW_HR(E_NOT_VALID_STATE);
    }

    std::unique_ptr<SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<SearchResultsTable>(connection);
    }

    std::vector<MatchType> Interface::GetMatchTypeOrder(MatchType type) const
    {
        // TODO: Move to shared implementation at least for this default set
        switch (type)
        {
        case MatchType::Exact:
            return { MatchType::Exact };
        case MatchType::CaseInsensitive:
            return { MatchType::CaseInsensitive };
        case MatchType::StartsWith:
            return { MatchType::CaseInsensitive, MatchType::StartsWith };
        case MatchType::Substring:
            return { MatchType::CaseInsensitive, MatchType::Substring };
        case MatchType::Wildcard:
            return { MatchType::Wildcard };
        case MatchType::Fuzzy:
            return { MatchType::CaseInsensitive, MatchType::Fuzzy };
        case MatchType::FuzzySubstring:
            return { MatchType::CaseInsensitive, MatchType::Fuzzy, MatchType::Substring, MatchType::FuzzySubstring };
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    void Interface::PerformQuerySearch(SearchResultsTable& resultsTable, const RequestMatch& query) const
    {
        // Arbitrary values to create a reusable filter with the given value.
        PackageMatchFilter filter(PackageMatchField::Id, MatchType::Exact, query.Value);

        for (MatchType match : GetMatchTypeOrder(query.Type))
        {
            filter.Type = match;

            for (auto field : { PackageMatchField::Id, PackageMatchField::Name, PackageMatchField::Moniker, PackageMatchField::Command, PackageMatchField::Tag })
            {
                filter.Field = field;
                resultsTable.SearchOnField(filter);
            }
        }
    }

    OneToManyTableSchema Interface::GetOneToManyTableSchema() const
    {
        return OneToManyTableSchema::Version_2_0;
    }

    ISQLiteIndex::SearchResult Interface::SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const
    {
        // TODO: Copy search and any modifications 1.1-1.7 here
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        // TODO: Write out changed package intermediate files and save their hashes

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v2_0");

        // TODO: Create 2.0 tables

        // TODO: Copy data from 1.N tables to 2.0 tables

        // TODO: Drop 1.N tables

        // TODO: Remove metadata for internal schema version to mark completion

        savepoint.Commit();

        if (vacuum)
        {
            Vacuum(connection);
        }
    }

    void Interface::Vacuum(const SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Vacuum();
        builder.Execute(connection);
    }

    void Interface::EnsureInternalInterface(const SQLite::Connection& connection, bool requireInternalInterface) const
    {
        if (!m_internalInterfaceChecked)
        {
            std::optional<int> major = SQLite::MetadataTable::TryGetNamedValue<int>(connection, s_MetadataValueName_InternalMajorVersion);
            std::optional<int> minor = SQLite::MetadataTable::TryGetNamedValue<int>(connection, s_MetadataValueName_InternalMinorVersion);

            if (major && minor)
            {
                m_internalInterface = CreateISQLiteIndex(SQLite::Version{ static_cast<uint32_t>(major.value()), static_cast<uint32_t>(minor.value()) });
            }

            m_internalInterfaceChecked = true;
        }

        THROW_HR_IF(E_NOT_VALID_STATE, requireInternalInterface && !m_internalInterface);
    }
}
