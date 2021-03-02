// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/Interface.h"

#include "Microsoft/Schema/1_0/IdTable.h"
#include "Microsoft/Schema/1_0/NameTable.h"
#include "Microsoft/Schema/1_0/MonikerTable.h"
#include "Microsoft/Schema/1_0/VersionTable.h"
#include "Microsoft/Schema/1_0/ChannelTable.h"

#include "Microsoft/Schema/1_0/PathPartTable.h"

#include "Microsoft/Schema/1_0/ManifestTable.h"

#include "Microsoft/Schema/1_0/TagsTable.h"
#include "Microsoft/Schema/1_0/CommandsTable.h"

#include "Microsoft/Schema/1_0/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    namespace
    {
        // Gets an existing manifest by its rowid., if it exists.
        std::optional<SQLite::rowid_t> GetExistingManifestId(SQLite::Connection& connection, const Manifest::Manifest& manifest)
        {
            std::optional<SQLite::rowid_t> idId = IdTable::SelectIdByValue(connection, manifest.Id, true);
            if (!idId)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find an Id { " << manifest.Id << " }");
                return {};
            }

            std::optional<SQLite::rowid_t> versionId = VersionTable::SelectIdByValue(connection, manifest.Version, true);
            if (!versionId)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find a Version { " << manifest.Version << " }");
                return {};
            }

            std::optional<SQLite::rowid_t> channelId = ChannelTable::SelectIdByValue(connection, manifest.Channel, true);
            if (!channelId)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find a Channel { " << manifest.Channel << " }");
                return {};
            }

            auto result = ManifestTable::SelectByValueIds<IdTable, VersionTable, ChannelTable>(connection, { idId.value(), versionId.value(), channelId.value() });

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find a manifest row for { " << manifest.Id << ", " << manifest.Version << ", " << manifest.Channel << " }");
            }

            return result;
        }

        // Gets a manifest id by the given key values.
        std::optional<SQLite::rowid_t> StaticGetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version = "", std::string_view channel = "")
        {
            std::optional<SQLite::rowid_t> channelIdOpt = ChannelTable::SelectIdByValue(connection, channel, true);
            if (!channelIdOpt && !channel.empty())
            {
                // If an empty channel was given but none was found, we will just not filter on channel.
                AICLI_LOG(Repo, Info, << "Did not find a Channel { " << channel << " }");
                return {};
            }

            std::optional<SQLite::rowid_t> versionIdOpt;

            if (version.empty())
            {
                std::vector<std::string> versionStrings;
                
                if (channelIdOpt)
                {
                    versionStrings = ManifestTable::GetAllValuesByIds<VersionTable, IdTable, ChannelTable>(connection, { id, channelIdOpt.value() });
                }
                else
                {
                    versionStrings = ManifestTable::GetAllValuesByIds<VersionTable, IdTable>(connection, { id });
                }

                if (versionStrings.empty())
                {
                    AICLI_LOG(Repo, Info, << "Did not find any Versions { " << id << ", " << channel << " }");
                    return {};
                }

                // Convert the strings to Versions and sort them
                std::vector<Utility::Version> versions;
                for (std::string& v : versionStrings)
                {
                    versions.emplace_back(std::move(v));
                }

                std::sort(versions.begin(), versions.end());

                // Get the last version in the list (the highest version) and its rowid
                const std::string& latestVersion = versions.back().ToString();
                versionIdOpt = VersionTable::SelectIdByValue(connection, latestVersion);
            }
            else
            {
                versionIdOpt = VersionTable::SelectIdByValue(connection, version, true);
            }

            if (!versionIdOpt)
            {
                AICLI_LOG(Repo, Info, << "Did not find a Version { " << version << " }");
                return {};
            }

            if (channelIdOpt)
            {
                return ManifestTable::SelectByValueIds<IdTable, VersionTable, ChannelTable>(connection, { id, versionIdOpt.value(), channelIdOpt.value() });
            }
            else
            {
                return ManifestTable::SelectByValueIds<IdTable, VersionTable>(connection, { id, versionIdOpt.value() });
            }
        }

        // Updates the manifest column and related table based on the given value.
        template <typename Table>
        void UpdateManifestValueById(SQLite::Connection& connection, const typename Table::value_t& value, SQLite::rowid_t manifestId, bool overwriteLikeMatch = false)
        {
            auto [oldValueId] = ManifestTable::GetIdsById<Table>(connection, manifestId);

            SQLite::rowid_t newValueId = Table::EnsureExists(connection, value, overwriteLikeMatch);

            ManifestTable::UpdateValueIdById<Table>(connection, manifestId, newValueId);

            Table::DeleteIfNotNeededById(connection, oldValueId);
        }
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 0 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_0");

        IdTable::Create_deprecated(connection);
        NameTable::Create_deprecated(connection);
        MonikerTable::Create_deprecated(connection);
        VersionTable::Create_deprecated(connection);
        ChannelTable::Create_deprecated(connection);

        PathPartTable::Create_deprecated(connection);

        ManifestTable::Create_deprecated(connection, {
            { IdTable::ValueName(), true, false }, 
            { NameTable::ValueName(), false, false },
            { MonikerTable::ValueName(), false, false },
            { VersionTable::ValueName(), true, false },
            { ChannelTable::ValueName(), true, false },
            { PathPartTable::ValueName(), false, true }
            });

        TagsTable::Create_deprecated(connection);
        CommandsTable::Create_deprecated(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        auto manifestResult = GetExistingManifestId(connection, manifest);

        // If this manifest is already present, we can't add it.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), manifestResult.has_value());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_0");

        auto [pathAdded, pathLeafId] = PathPartTable::EnsurePathExists(connection, relativePath, true);

        // If we get false from the function, this manifest path already exists in the index.
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), !pathAdded);

        // Ensure that all of the 1:1 data exists.
        SQLite::rowid_t idId = IdTable::EnsureExists(connection, manifest.Id, true);
        SQLite::rowid_t nameId = NameTable::EnsureExists(connection, manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>());
        SQLite::rowid_t monikerId = MonikerTable::EnsureExists(connection, manifest.Moniker);
        SQLite::rowid_t versionId = VersionTable::EnsureExists(connection, manifest.Version);
        SQLite::rowid_t channelId = ChannelTable::EnsureExists(connection, manifest.Channel);

        // Insert the manifest entry.
        SQLite::rowid_t manifestId = ManifestTable::Insert(connection, {
            { IdTable::ValueName(), idId},
            { NameTable::ValueName(), nameId },
            { MonikerTable::ValueName(), monikerId },
            { VersionTable::ValueName(), versionId },
            { ChannelTable::ValueName(), channelId },
            { PathPartTable::ValueName(), pathLeafId }
            });

        // Add all of the 1:N data.
        TagsTable::EnsureExistsAndInsert(connection, manifest.GetAggregatedTags(), manifestId);
        CommandsTable::EnsureExistsAndInsert(connection, manifest.GetAggregatedCommands(), manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        auto manifestResult = GetExistingManifestId(connection, manifest);

        // If the manifest doesn't actually exist, fail the update.
        THROW_HR_IF(E_NOT_SET, !manifestResult);

        SQLite::rowid_t manifestId = manifestResult.value();

        auto [idInIndex, nameInIndex, monikerInIndex, versionInIndex, channelInIndex] =
            ManifestTable::GetValuesById<IdTable, NameTable, MonikerTable, VersionTable, ChannelTable>(connection, manifestId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_0");
        bool indexModified = false;

        // Id, Version, and Channel may have changed casing. If so, they too need to be updated.
        if (idInIndex != manifest.Id)
        {
            UpdateManifestValueById<IdTable>(connection, manifest.Id, manifestId, true);
            indexModified = true;
        }

        if (versionInIndex != manifest.Version)
        {
            UpdateManifestValueById<VersionTable>(connection, manifest.Version, manifestId);
            indexModified = true;
        }

        if (channelInIndex != manifest.Channel)
        {
            UpdateManifestValueById<ChannelTable>(connection, manifest.Channel, manifestId);
            indexModified = true;
        }

        auto packageName = manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>();
        if (nameInIndex != packageName)
        {
            UpdateManifestValueById<NameTable>(connection, packageName, manifestId);
            indexModified = true;
        }

        if (monikerInIndex != manifest.Moniker)
        {
            UpdateManifestValueById<MonikerTable>(connection, manifest.Moniker, manifestId);
            indexModified = true;
        }

        // Update path table if necessary
        auto [existingPathLeafId] = ManifestTable::GetIdsById<PathPartTable>(connection, manifestId);
        auto [pathAdded, newPathLeafId] = PathPartTable::EnsurePathExists(connection, relativePath, true);

        if (pathAdded)
        {
            // Path was added, so we need to update the manifest table and delete the old path
            ManifestTable::UpdateValueIdById<PathPartTable>(connection, manifestId, newPathLeafId);
            PathPartTable::RemovePathById(connection, existingPathLeafId);
            indexModified = true;
        }
        else
        {
            // The path already existed, so it must either match the existing manifest path or it is an error
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), existingPathLeafId != newPathLeafId);
        }

        // Update all 1:N tables as necessary
        indexModified = TagsTable::UpdateIfNeededByManifestId(connection, manifest.GetAggregatedTags(), manifestId) || indexModified;
        indexModified = CommandsTable::UpdateIfNeededByManifestId(connection, manifest.GetAggregatedCommands(), manifestId) || indexModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path&)
    {
        auto manifestResult = GetExistingManifestId(connection, manifest);

        // If the manifest doesn't actually exist, fail the remove.
        THROW_HR_IF(E_NOT_SET, !manifestResult);

        SQLite::rowid_t manifestId = manifestResult.value();

        // Get the ids of the values from the manifest table
        auto [idId, nameId, monikerId, versionId, channelId, pathLeafId] = 
            ManifestTable::GetIdsById<IdTable, NameTable, MonikerTable, VersionTable, ChannelTable, PathPartTable>(connection, manifestId);

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_0");

        // Remove the manifest row
        ManifestTable::DeleteById(connection, manifestId);

        // Remove all of the 1:1 data that is no longer referenced.
        IdTable::DeleteIfNotNeededById(connection, idId);
        NameTable::DeleteIfNotNeededById(connection, nameId);
        MonikerTable::DeleteIfNotNeededById(connection, monikerId);
        VersionTable::DeleteIfNotNeededById(connection, versionId);
        ChannelTable::DeleteIfNotNeededById(connection, channelId);

        // Remove the path
        PathPartTable::RemovePathById(connection, pathLeafId);

        // Remove all of the 1:N data that is no longer referenced.
        TagsTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        CommandsTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        savepoint.Commit();

        return manifestId;
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_0");

        IdTable::PrepareForPackaging_deprecated(connection);
        NameTable::PrepareForPackaging_deprecated(connection);
        MonikerTable::PrepareForPackaging_deprecated(connection);
        VersionTable::PrepareForPackaging_deprecated(connection);
        ChannelTable::PrepareForPackaging_deprecated(connection);

        PathPartTable::PrepareForPackaging_deprecated(connection);

        ManifestTable::PrepareForPackaging_deprecated(connection, {
            VersionTable::ValueName(),
            ChannelTable::ValueName(),
            PathPartTable::ValueName(),
            });

        TagsTable::PrepareForPackaging_deprecated(connection);
        CommandsTable::PrepareForPackaging_deprecated(connection);

        savepoint.Commit();

        // Force the database to actually shrink the file size.
        // This *must* be done outside of an active transaction.
        SQLite::Builder::StatementBuilder builder;
        builder.Vacuum();
        builder.Execute(connection);
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = true;

        // Check the manifest table references to it's 1:1 tables
        if (result || log)
        {
            result = ManifestTable::CheckConsistency<IdTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = ManifestTable::CheckConsistency<NameTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = ManifestTable::CheckConsistency<MonikerTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = ManifestTable::CheckConsistency<VersionTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = ManifestTable::CheckConsistency<ChannelTable>(connection, log) && result;
        }

        if (result || log)
        {
            result = ManifestTable::CheckConsistency<PathPartTable>(connection, log) && result;
        }

        // Check the pathpaths table for consistency
        if (result || log)
        {
            result = PathPartTable::CheckConsistency(connection, log) && result;
        }

        // Check the 1:N map tables for consistency
        if (result || log)
        {
            result = TagsTable::CheckConsistency(connection, log) && result;
        }

        if (result || log)
        {
            result = CommandsTable::CheckConsistency(connection, log) && result;
        }

        return result;
    }

    ISQLiteIndex::SearchResult Interface::Search(const SQLite::Connection& connection, const SearchRequest& request) const
    {
        if (request.IsForEverything())
        {
            std::vector<SQLite::rowid_t> ids = IdTable::GetAllRowIds(connection, request.MaximumResults);

            SearchResult result;
            for (SQLite::rowid_t id : ids)
            {
                result.Matches.emplace_back(std::make_pair(id, PackageMatchFilter(PackageMatchField::Id, MatchType::Wildcard)));
            }

            result.Truncated = (request.MaximumResults && IdTable::GetCount(connection) > request.MaximumResults);

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
        if (!ManifestTable::ExistsById(connection, manifestId))
        {
            AICLI_LOG(Repo, Info, << "Did not find manifest by id: " << manifestId);
            return {};
        }

        switch (property)
        {
        case AppInstaller::Repository::PackageVersionProperty::Id:
            return std::get<0>(ManifestTable::GetValuesById<IdTable>(connection, manifestId));
        case AppInstaller::Repository::PackageVersionProperty::Name:
            return std::get<0>(ManifestTable::GetValuesById<NameTable>(connection, manifestId));
        case AppInstaller::Repository::PackageVersionProperty::Version:
            return std::get<0>(ManifestTable::GetValuesById<VersionTable>(connection, manifestId));
        case AppInstaller::Repository::PackageVersionProperty::Channel:
            return std::get<0>(ManifestTable::GetValuesById<ChannelTable>(connection, manifestId));
        case AppInstaller::Repository::PackageVersionProperty::RelativePath:
            return PathPartTable::GetPathById(connection, std::get<0>(ManifestTable::GetIdsById<PathPartTable>(connection, manifestId)));
        default:
            return {};
        }
    }

    std::vector<std::string> Interface::GetMultiPropertyByManifestId(const SQLite::Connection&, SQLite::rowid_t, PackageVersionMultiProperty) const
    {
        return {};
    }

    std::optional<SQLite::rowid_t> Interface::GetManifestIdByKey(const SQLite::Connection& connection, SQLite::rowid_t id, std::string_view version, std::string_view channel) const
    {
        return StaticGetManifestIdByKey(connection, id, version, channel);
    }

    std::vector<Utility::VersionAndChannel> Interface::GetVersionKeysById(const SQLite::Connection& connection, SQLite::rowid_t id) const
    {
        auto versionsAndChannels = ManifestTable::GetAllValuesById<IdTable, VersionTable, ChannelTable>(connection, id);

        std::vector<Utility::VersionAndChannel> result;
        result.reserve(versionsAndChannels.size());
        for (auto&& vac : versionsAndChannels)
        {
            result.emplace_back(Utility::Version{ std::move(std::get<0>(vac)) }, Utility::Channel{ std::move(std::get<1>(vac)) });
        }

        std::sort(result.begin(), result.end());

        return result;
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
        Utility::NormalizedName result;
        result.Name(name);
        result.Publisher(publisher);
        return result;
    }

    std::unique_ptr<SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<SearchResultsTable>(connection);
    }

    std::vector<MatchType> Interface::GetMatchTypeOrder(MatchType type) const
    {
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
}
