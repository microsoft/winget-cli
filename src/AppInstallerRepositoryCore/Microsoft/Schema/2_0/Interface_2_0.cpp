// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/SQLiteMetadataTable.h>
#include "Microsoft/Schema/2_0/Interface.h"

#include "Microsoft/Schema/2_0/PackagesTable.h"

#include "Microsoft/Schema/2_0/TagsTable.h"
#include "Microsoft/Schema/2_0/CommandsTable.h"
#include "Microsoft/Schema/2_0/PackageFamilyNameTable.h"
#include "Microsoft/Schema/2_0/ProductCodeTable.h"
#include "Microsoft/Schema/2_0/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/2_0/NormalizedPackagePublisherTable.h"
#include "Microsoft/Schema/2_0/UpgradeCodeTable.h"

#include "Microsoft/Schema/2_0/SearchResultsTable.h"
#include "Microsoft/Schema/2_0/PackageUpdateTrackingTable.h"

#include <winget/PackageVersionDataManifest.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace anon
    {
        // Folds the values of the fields that are stored folded.
        void FoldPackageMatchFilters(std::vector<PackageMatchFilter>& filters)
        {
            for (auto& filter : filters)
            {
                if ((filter.Field == PackageMatchField::PackageFamilyName || filter.Field == PackageMatchField::ProductCode || filter.Field == PackageMatchField::UpgradeCode) &&
                    filter.Type == MatchType::Exact)
                {
                    filter.Value = Utility::FoldCase(filter.Value);
                }
            }
        }

        // Update NormalizedNameAndPublisher with normalization and folding
        // Returns true if the normalized name contains normalization field of fieldsToInclude
        bool UpdateNormalizedNameAndPublisher(
            PackageMatchFilter& filter,
            const Utility::NameNormalizer& normalizer,
            Utility::NormalizationField fieldsToInclude)
        {
            Utility::NormalizedName normalized = normalizer.Normalize(Utility::FoldCase(filter.Value), Utility::FoldCase(filter.Additional.value()));
            filter.Value = normalized.GetNormalizedName(fieldsToInclude);
            filter.Additional = normalized.Publisher();
            return WI_AreAllFlagsSet(normalized.GetNormalizedFields(), fieldsToInclude);
        }

        bool UpdatePackageMatchFilters(
            std::vector<PackageMatchFilter>& filters,
            const Utility::NameNormalizer& normalizer,
            Utility::NormalizationField normalizedNameFieldsToFilter = Utility::NormalizationField::None)
        {
            bool normalizedNameFieldsFound = false;
            for (auto itr = filters.begin(); itr != filters.end();)
            {
                if (itr->Field == PackageMatchField::NormalizedNameAndPublisher && itr->Type == MatchType::Exact)
                {
                    if (!UpdateNormalizedNameAndPublisher(*itr, normalizer, normalizedNameFieldsToFilter))
                    {
                        // If not matched, this package match filter will be removed.
                        // For example, if caller is trying to search with arch info only, values without arch will be removed from search.
                        itr = filters.erase(itr);
                        continue;
                    }

                    normalizedNameFieldsFound = true;
                }

                ++itr;
            }

            return normalizedNameFieldsFound;
        }
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
        m_internalInterface = CreateInternalInterface();

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v2_0");

        // We only create the internal tables at this point, the actual 2.0 tables are created in PrepareForPackaging
        m_internalInterface->CreateTables(connection, options);

        savepoint.Commit();

        m_internalInterfaceChecked = true;
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        EnsureInternalInterface(connection, true);
        SQLite::rowid_t manifestId = m_internalInterface->AddManifest(connection, manifest, relativePath);
        PackageUpdateTrackingTable::Update(connection, m_internalInterface.get(), m_internalInterface->GetPropertyByPrimaryId(connection, manifestId, PackageVersionProperty::Id).value());
        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        EnsureInternalInterface(connection, true);
        std::pair<bool, SQLite::rowid_t> result = m_internalInterface->UpdateManifest(connection, manifest, relativePath);
        if (result.first)
        {
            PackageUpdateTrackingTable::Update(connection, m_internalInterface.get(), m_internalInterface->GetPropertyByPrimaryId(connection, result.second, PackageVersionProperty::Id).value());
        }
        return result;
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest)
    {
        EnsureInternalInterface(connection, true);
        std::optional<SQLite::rowid_t> result = m_internalInterface->GetManifestIdByManifest(connection, manifest);

        // If the manifest doesn't actually exist, fail the remove.
        THROW_HR_IF(E_NOT_SET, !result);

        SQLite::rowid_t manifestId = result.value();
        RemoveManifestById(connection, manifestId);

        return manifestId;
    }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        EnsureInternalInterface(connection, true);
        std::optional<std::string> identifier = m_internalInterface->GetPropertyByPrimaryId(connection, manifestId, PackageVersionProperty::Id);
        m_internalInterface->RemoveManifestById(connection, manifestId);
        if (identifier)
        {
            PackageUpdateTrackingTable::Update(connection, m_internalInterface.get(), identifier.value());
        }
    }

    void Interface::PrepareForPackaging(SQLite::Connection&)
    {
        // We implement the context version
        THROW_HR(E_NOTIMPL);
    }

    void Interface::PrepareForPackaging(const SQLiteIndexContext& context)
    {
        EnsureInternalInterface(context.Connection, true);
        PrepareForPackaging(context, true);
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        EnsureInternalInterface(connection);

        bool result = true;

#define AICLI_CHECK_CONSISTENCY(_check_) \
        if (result || log) \
        { \
            result = _check_ && result; \
        }

        if (m_internalInterface)
        {
            AICLI_CHECK_CONSISTENCY(m_internalInterface->CheckConsistency(connection, log));
            AICLI_CHECK_CONSISTENCY(PackageUpdateTrackingTable::CheckConsistency(connection, m_internalInterface.get(), log));

            return result;
        }

        AICLI_CHECK_CONSISTENCY((PackagesTable::CheckConsistency<
            PackagesTable::IdColumn,
            PackagesTable::NameColumn,
            PackagesTable::MonikerColumn,
            PackagesTable::LatestVersionColumn,
            PackagesTable::ARPMinVersionColumn,
            PackagesTable::ARPMaxVersionColumn>(connection, log)));

        // Check the 1:N map tables for consistency
        AICLI_CHECK_CONSISTENCY(TagsTable::CheckConsistency(connection, log));
        AICLI_CHECK_CONSISTENCY(CommandsTable::CheckConsistency(connection, log));

        AICLI_CHECK_CONSISTENCY(PackageFamilyNameTable::CheckConsistency(connection, log));
        AICLI_CHECK_CONSISTENCY(ProductCodeTable::CheckConsistency(connection, log));
        AICLI_CHECK_CONSISTENCY(NormalizedPackageNameTable::CheckConsistency(connection, log));
        AICLI_CHECK_CONSISTENCY(NormalizedPackagePublisherTable::CheckConsistency(connection, log));
        AICLI_CHECK_CONSISTENCY(UpgradeCodeTable::CheckConsistency(connection, log));

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

        SearchRequest requestCopy = request;
        return SearchInternal(connection, requestCopy);
    }

    std::optional<std::string> Interface::GetPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionProperty property) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetPropertyByPrimaryId(connection, primaryId, property);
        }

        switch (property)
        {
        case PackageVersionProperty::Id:
            return PackagesTable::GetValueById<PackagesTable::IdColumn>(connection, primaryId);
        case PackageVersionProperty::Name:
            return PackagesTable::GetValueById<PackagesTable::NameColumn>(connection, primaryId);
        case PackageVersionProperty::Version:
            return PackagesTable::GetValueById<PackagesTable::LatestVersionColumn>(connection, primaryId);
        case PackageVersionProperty::Channel:
            return "";
        case PackageVersionProperty::ManifestSHA256Hash:
        {
            std::optional<SQLite::blob_t> hash = PackagesTable::GetValueById<PackagesTable::HashColumn>(connection, primaryId);
            return (!hash || hash->empty()) ? std::optional<std::string>{} : Utility::SHA256::ConvertToString(hash.value());
        }
        case PackageVersionProperty::ArpMinVersion:
            return PackagesTable::GetValueById<PackagesTable::ARPMinVersionColumn>(connection, primaryId);
        case PackageVersionProperty::ArpMaxVersion:
            return PackagesTable::GetValueById<PackagesTable::ARPMaxVersionColumn>(connection, primaryId);
        case PackageVersionProperty::Moniker:
            return PackagesTable::GetValueById<PackagesTable::MonikerColumn>(connection, primaryId);
        default:
            return {};
        }
    }

    std::vector<std::string> Interface::GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->GetMultiPropertyByPrimaryId(connection, primaryId, property);
        }

        switch (property)
        {
        case PackageVersionMultiProperty::PackageFamilyName:
            return PackageFamilyNameTable::GetValuesByPrimaryId(connection, primaryId);
        case PackageVersionMultiProperty::ProductCode:
            return ProductCodeTable::GetValuesByPrimaryId(connection, primaryId);
            // These values are not right, as they are normalized.  But they are good enough for now and all we have.
        case PackageVersionMultiProperty::Name:
            return NormalizedPackageNameTable::GetValuesByPrimaryId(connection, primaryId);
        case PackageVersionMultiProperty::Publisher:
            return NormalizedPackagePublisherTable::GetValuesByPrimaryId(connection, primaryId);
        case PackageVersionMultiProperty::UpgradeCode:
            return UpgradeCodeTable::GetValuesByPrimaryId(connection, primaryId);
        case PackageVersionMultiProperty::Tag:
            return TagsTable::GetValuesByPrimaryId(connection, primaryId);
        case PackageVersionMultiProperty::Command:
            return CommandsTable::GetValuesByPrimaryId(connection, primaryId);
        default:
            return {};
        }
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
        if (m_internalInterface)
        {
            return m_internalInterface->NormalizeName(name, publisher);
        }

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

    void Interface::DropTables(SQLite::Connection& connection)
    {
        EnsureInternalInterface(connection);

        if (m_internalInterface)
        {
            return m_internalInterface->DropTables(connection);
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "drop_tables_v2_0");

        PackagesTable::Drop(connection);

        TagsTable::Drop(connection);
        CommandsTable::Drop(connection);

        PackageFamilyNameTable::Drop(connection);
        ProductCodeTable::Drop(connection);
        NormalizedPackageNameTable::Drop(connection);
        NormalizedPackagePublisherTable::Drop(connection);
        UpgradeCodeTable::Drop(connection);

        savepoint.Commit();
    }

    bool Interface::MigrateFrom(SQLite::Connection& connection, const ISQLiteIndex* current)
    {
        THROW_HR_IF_NULL(E_POINTER, current);

        auto currentVersion = current->GetVersion();
        if (currentVersion.MajorVersion != 1 || currentVersion.MinorVersion != 7)
        {
            return false;
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "migrate_from_v2_0");

        // We only need to insert all of the existing packages into the update tracking table.
        PackageUpdateTrackingTable::EnsureExists(connection);
        SearchResult allPackages = current->Search(connection, {});

        for (const auto& packageMatch : allPackages.Matches)
        {
            std::vector<ISQLiteIndex::VersionKey> versionKeys = current->GetVersionKeysById(connection, packageMatch.first);
            ISQLiteIndex::VersionKey& latestVersionKey = versionKeys[0];
            PackageUpdateTrackingTable::Update(connection, current, current->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::Id).value(), false);
        }

        savepoint.Commit();
        return true;
    }

    void Interface::SetProperty(SQLite::Connection& connection, Property property, const std::string& value)
    {
        switch (property)
        {
        case Property::PackageUpdateTrackingBaseTime:
        {
            int64_t baseTime = 0;
            if (value.empty())
            {
                baseTime = Utility::GetCurrentUnixEpoch();
            }
            else
            {
                baseTime = std::stoll(value);
            }
            SQLite::MetadataTable::SetNamedValue(connection, s_MetadataValueName_PackageUpdateTrackingBaseTime, std::to_string(baseTime));
        }
            break;

        default:
            THROW_WIN32(ERROR_NOT_SUPPORTED);
        }
    }

    std::unique_ptr<SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<SearchResultsTable>(connection);
    }

    void Interface::PerformQuerySearch(SearchResultsTable& resultsTable, const RequestMatch& query) const
    {
        // First, do an exact match search for the folded system reference strings
        // We do this first because it is exact, and likely won't match anything else if it matches this.
        PackageMatchFilter filter(PackageMatchField::Unknown, MatchType::Exact, Utility::FoldCase(query.Value));

        for (PackageMatchField field : { PackageMatchField::PackageFamilyName, PackageMatchField::ProductCode, PackageMatchField::UpgradeCode })
        {
            filter.Field = field;
            resultsTable.SearchOnField(filter);
        }

        // Now search on the unfolded value
        filter.Value = query.Value;

        for (MatchType match : GetDefaultMatchTypeOrder(query.Type))
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
        anon::FoldPackageMatchFilters(request.Inclusions);
        anon::FoldPackageMatchFilters(request.Filters);

        if (request.Purpose == SearchPurpose::CorrelationToInstalled)
        {
            // Correlate from available package to installed package
            // For available package to installed package mapping, only one try is needed.
            // For example, if ARP DisplayName contains arch, then the installed package's ARP DisplayName should also include arch.
            auto candidateInclusionsWithArch = request.Inclusions;
            if (anon::UpdatePackageMatchFilters(candidateInclusionsWithArch, m_normalizer, Utility::NormalizationField::Architecture))
            {
                // If DisplayNames contain arch, only use Inclusions with arch for search
                request.Inclusions = candidateInclusionsWithArch;
            }
            else
            {
                // Otherwise, just update the Inclusions with normalization
                anon::UpdatePackageMatchFilters(request.Inclusions, m_normalizer);
            }

            return BasicSearchInternal(connection, request);
        }
        else if (request.Purpose == SearchPurpose::CorrelationToAvailable)
        {
            // For installed package to available package correlation,
            // try the search with NormalizedName with Arch first, if not found, try with all values.
            // This can be extended in the future for more granular search requests.
            std::vector<SearchRequest> candidateSearches;
            auto candidateSearchWithArch = request;
            if (anon::UpdatePackageMatchFilters(candidateSearchWithArch.Inclusions, m_normalizer, Utility::NormalizationField::Architecture))
            {
                candidateSearches.emplace_back(std::move(candidateSearchWithArch));
            }
            anon::UpdatePackageMatchFilters(request.Inclusions, m_normalizer);
            candidateSearches.emplace_back(request);

            SearchResult result;
            for (auto& candidateSearch : candidateSearches)
            {
                result = BasicSearchInternal(connection, candidateSearch);
                if (!result.Matches.empty())
                {
                    break;
                }
            }

            return result;
        }
        else
        {
            anon::UpdatePackageMatchFilters(request.Inclusions, m_normalizer);
            anon::UpdatePackageMatchFilters(request.Filters, m_normalizer);

            return BasicSearchInternal(connection, request);
        }
    }

    ISQLiteIndex::SearchResult Interface::BasicSearchInternal(const SQLite::Connection& connection, const SearchRequest& request) const
    {
        if (request.IsForEverything())
        {
            std::vector<SQLite::rowid_t> ids = PackagesTable::GetAllRowIds(connection, PackagesTable::IdColumn::Name, request.MaximumResults);

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
                for (MatchType match : GetDefaultMatchTypeOrder(include.Type))
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

            for (MatchType match : GetDefaultMatchTypeOrder(filter.Type))
            {
                filter.Type = match;
                resultsTable->SearchOnField(filter);
            }

            // Skip the filter as we already know everything matches
            filterIndex = 1;
        }

        // Remove any duplicate manifest entries
        resultsTable->RemoveDuplicatePackageRows();

        // Second phase, for remaining filters, flag matching search results, then remove unflagged values.
        for (size_t i = filterIndex; i < request.Filters.size(); ++i)
        {
            PackageMatchFilter filter = request.Filters[i];

            resultsTable->PrepareToFilter();

            for (MatchType match : GetDefaultMatchTypeOrder(filter.Type))
            {
                filter.Type = match;
                resultsTable->FilterOnField(filter);
            }

            resultsTable->CompleteFilter();
        }

        return resultsTable->GetSearchResults(request.MaximumResults);
    }

    void Interface::PrepareForPackaging(const SQLiteIndexContext& context, bool vacuum)
    {
        SQLite::Connection& connection = context.Connection;

        // Get the base time from metadata
        int64_t updateBaseTime = 0;
        std::optional<std::string> updateBaseTimeString = SQLite::MetadataTable::TryGetNamedValue<std::string>(connection, s_MetadataValueName_PackageUpdateTrackingBaseTime);
        if (updateBaseTimeString && !updateBaseTimeString->empty())
        {
            updateBaseTime = std::stoll(updateBaseTimeString.value());
        }

        // Get the output directory or use the file path
        std::filesystem::path baseOutputDirectory;

        if (context.Data.Contains(Property::IntermediateFileOutputPath))
        {
            baseOutputDirectory = context.Data.Get<Property::IntermediateFileOutputPath>();
        }
        else if (context.Data.Contains(Property::DatabaseFilePath))
        {
            baseOutputDirectory = context.Data.Get<Property::DatabaseFilePath>();
            baseOutputDirectory = baseOutputDirectory.parent_path();
        }

        THROW_WIN32_IF(ERROR_INVALID_STATE, baseOutputDirectory.empty() || baseOutputDirectory.is_relative());

        // Output all of the changed package version manifests since the base time to the target location
        for (const auto& packageData : PackageUpdateTrackingTable::GetUpdatesSince(connection, updateBaseTime))
        {
            std::filesystem::path packageDirectory = baseOutputDirectory /
                Manifest::PackageVersionDataManifest::GetRelativeDirectoryPath(packageData.PackageIdentifier, Utility::SHA256::ConvertToString(packageData.Hash));

            std::filesystem::create_directories(packageDirectory);

            std::filesystem::path manifestPath = packageDirectory / Manifest::PackageVersionDataManifest::VersionManifestCompressedFileName();

            AICLI_LOG(Repo, Info, << "Writing PackageVersionDataManifest for [" << packageData.PackageIdentifier << "] to [" << manifestPath << "]");

            std::ofstream stream(manifestPath, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            THROW_LAST_ERROR_IF(stream.fail());
            stream.write(reinterpret_cast<const char*>(packageData.Manifest.data()), packageData.Manifest.size());
            THROW_LAST_ERROR_IF(stream.fail());
            stream.flush();
        }

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v2_0");

        // Create the 2.0 data tables
        PackagesTable::Create<
            PackagesTable::IdColumn,
            PackagesTable::NameColumn,
            PackagesTable::MonikerColumn,
            PackagesTable::LatestVersionColumn,
            PackagesTable::ARPMinVersionColumn,
            PackagesTable::ARPMaxVersionColumn,
            PackagesTable::HashColumn
        >(connection);

        TagsTable::Create(connection, GetOneToManyTableSchema());
        CommandsTable::Create(connection, GetOneToManyTableSchema());

        PackageFamilyNameTable::Create(connection);
        ProductCodeTable::Create(connection);
        NormalizedPackageNameTable::Create(connection);
        NormalizedPackagePublisherTable::Create(connection);
        UpgradeCodeTable::Create(connection);

        // Copy data from 1.7 tables to 2.0 tables
        SearchResult allPackages = m_internalInterface->Search(connection, {});

        for (const auto& packageMatch : allPackages.Matches)
        {
            std::vector<ISQLiteIndex::VersionKey> versionKeys = m_internalInterface->GetVersionKeysById(connection, packageMatch.first);
            ISQLiteIndex::VersionKey& latestVersionKey = versionKeys[0];

            std::string packageIdentifier = m_internalInterface->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::Id).value();

            std::vector<PackagesTable::NameValuePair> packageData{
                { PackagesTable::IdColumn::Name, packageIdentifier },
                { PackagesTable::NameColumn::Name, m_internalInterface->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::Name).value() },
                { PackagesTable::LatestVersionColumn::Name, latestVersionKey.VersionAndChannel.GetVersion().ToString() },
            };

            auto addIfPresent = [&](std::string_view name, std::optional<std::string>&& value)
                {
                    if (value && !value->empty())
                    {
                        packageData.emplace_back(PackagesTable::NameValuePair{ name, std::move(value).value() });
                    }
                };

            addIfPresent(PackagesTable::MonikerColumn::Name, m_internalInterface->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::Moniker).value());
            addIfPresent(PackagesTable::ARPMinVersionColumn::Name, m_internalInterface->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::ArpMinVersion).value());
            addIfPresent(PackagesTable::ARPMaxVersionColumn::Name, m_internalInterface->GetPropertyByPrimaryId(connection, latestVersionKey.ManifestId, PackageVersionProperty::ArpMaxVersion).value());

            SQLite::rowid_t packageId = PackagesTable::Insert(connection, packageData);

            PackagesTable::UpdateValueIdById<PackagesTable::HashColumn>(connection, packageId, PackageUpdateTrackingTable::GetDataHash(connection, packageIdentifier));

            for (const auto& versionKey : versionKeys)
            {
                TagsTable::EnsureExistsAndInsert(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::Tag), packageId);
                CommandsTable::EnsureExistsAndInsert(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::Command), packageId);

                PackageFamilyNameTable::EnsureExists(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::PackageFamilyName), packageId);
                ProductCodeTable::EnsureExists(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::ProductCode), packageId);
                NormalizedPackageNameTable::EnsureExists(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::Name), packageId);
                NormalizedPackagePublisherTable::EnsureExists(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::Publisher), packageId);
                UpgradeCodeTable::EnsureExists(connection, m_internalInterface->GetMultiPropertyByPrimaryId(connection, versionKey.ManifestId, PackageVersionMultiProperty::UpgradeCode), packageId);
            }
        }

        PackagesTable::PrepareForPackaging<
            PackagesTable::IdColumn,
            PackagesTable::NameColumn,
            PackagesTable::MonikerColumn,
            PackagesTable::LatestVersionColumn,
            PackagesTable::ARPMinVersionColumn,
            PackagesTable::ARPMaxVersionColumn,
            PackagesTable::HashColumn
        >(connection);

        TagsTable::PrepareForPackaging(connection);
        CommandsTable::PrepareForPackaging(connection);

        PackageUpdateTrackingTable::Drop(connection);

        // The tables based on SystemReferenceStringTable don't need a prepare currently

        // Drop 1.7 tables
        m_internalInterface->DropTables(connection);

        savepoint.Commit();

        m_internalInterface.reset();

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
            if (!PackagesTable::Exists(connection))
            {
                m_internalInterface = CreateInternalInterface();
            }

            m_internalInterfaceChecked = true;
        }

        THROW_HR_IF(E_NOT_VALID_STATE, requireInternalInterface && !m_internalInterface);
    }

    std::unique_ptr<Schema::ISQLiteIndex> Interface::CreateInternalInterface() const
    {
        return CreateISQLiteIndex({ 1, 7 });
    }
}
