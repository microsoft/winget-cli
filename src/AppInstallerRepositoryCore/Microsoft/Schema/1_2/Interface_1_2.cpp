// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_2/Interface.h"

#include "Microsoft/Schema/1_2/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/1_2/NormalizedPackagePublisherTable.h"

#include "Microsoft/Schema/1_2/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
{
    namespace anon
    {
        void AddNormalizedName(
            const Utility::NameNormalizer& normalizer,
            const Manifest::string_t& name,
            std::vector<Utility::NormalizedString>& out,
            Utility::NormalizationField fieldsToInclude = Utility::NormalizationField::None)
        {
            Utility::NormalizedString value = normalizer.NormalizeName(Utility::FoldCase(name)).GetNormalizedName(fieldsToInclude);
            if (std::find(out.begin(), out.end(), value) == out.end())
            {
                out.emplace_back(std::move(value));
            }
        }

        void AddLocalizationNormalizedName(const Utility::NameNormalizer& normalizer, const Manifest::ManifestLocalization& localization, std::vector<Utility::NormalizedString>& out)
        {
            if (localization.Contains(Manifest::Localization::PackageName))
            {
                AddNormalizedName(normalizer, localization.Get<Manifest::Localization::PackageName>(), out);
            }
        }

        void AddNormalizedPublisher(const Utility::NameNormalizer& normalizer, const Manifest::string_t& publisher, std::vector<Utility::NormalizedString>& out)
        {
            Utility::NormalizedString value = normalizer.NormalizePublisher(Utility::FoldCase(publisher));
            if (std::find(out.begin(), out.end(), value) == out.end())
            {
                out.emplace_back(std::move(value));
            }
        }

        void AddLocalizationNormalizedPublisher(const Utility::NameNormalizer& normalizer, const Manifest::ManifestLocalization& localization, std::vector<Utility::NormalizedString>& out)
        {
            if (localization.Contains(Manifest::Localization::Publisher))
            {
                AddNormalizedPublisher(normalizer, localization.Get<Manifest::Localization::Publisher>(), out);
            }
        }

        std::vector<Utility::NormalizedString> GetNormalizedNames(const Utility::NameNormalizer& normalizer, const Manifest::Manifest& manifest)
        {
            std::vector<Utility::NormalizedString> result;

            AddLocalizationNormalizedName(normalizer, manifest.DefaultLocalization, result);
            for (const auto& loc : manifest.Localizations)
            {
                AddLocalizationNormalizedName(normalizer, loc, result);
            }

            // In addition to the names used for our display, add the display names from the ARP entries
            for (const auto& installer : manifest.Installers)
            {
                for (const auto& appsAndFeaturesEntry : installer.AppsAndFeaturesEntries)
                {
                    if (!appsAndFeaturesEntry.DisplayName.empty())
                    {
                        AddNormalizedName(normalizer, appsAndFeaturesEntry.DisplayName, result);
                        // For arp display name, also add a copy with architecture info for more accurate correlation.
                        AddNormalizedName(normalizer, appsAndFeaturesEntry.DisplayName, result, Utility::NormalizationField::Architecture);
                    }
                }
            }

            return result;
        }

        std::vector<Utility::NormalizedString> GetNormalizedPublishers(const Utility::NameNormalizer& normalizer, const Manifest::Manifest& manifest)
        {
            std::vector<Utility::NormalizedString> result;

            AddLocalizationNormalizedPublisher(normalizer, manifest.DefaultLocalization, result);
            for (const auto& loc : manifest.Localizations)
            {
                AddLocalizationNormalizedPublisher(normalizer, loc, result);
            }

            // In addition to the publishers used for our display, add the publishers from the ARP entries
            for (const auto& installer : manifest.Installers)
            {
                for (const auto& appsAndFeaturesEntry : installer.AppsAndFeaturesEntries)
                {
                    if (!appsAndFeaturesEntry.Publisher.empty())
                    {
                        AddNormalizedPublisher(normalizer, appsAndFeaturesEntry.Publisher, result);
                    }
                }
            }

            return result;
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

        // Update NormalizedNameAndPublisher with normalization and folding
        // Returns true if any of normalized name contains normalization field of fieldsToInclude
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
        return { 1, 2 };
    }

    void Interface::CreateTables(SQLite::Connection& connection, CreateOptions options)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_2");

        V1_1::Interface::CreateTables(connection, options);

        // While the name and publisher should be linked per-locale, we are not implementing that here.
        // This will mean that one can match cross locale name and publisher, but the chance that this
        // leads to a confusion between packages is very small. More likely would be intentional attempts
        // to confuse the correlation, which could be fairly easily carried out even with linked values.
        NormalizedPackageNameTable::Create(connection, GetOneToManyTableSchema());
        NormalizedPackagePublisherTable::Create(connection, GetOneToManyTableSchema());

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_2");

        SQLite::rowid_t manifestId = V1_1::Interface::AddManifest(connection, manifest, relativePath);

        // Add the new 1.2 data
        // These normalized strings are all stored with their cases folded so that they can be
        // looked up ordinally; enabling the index to provide efficient searches.
        NormalizedPackageNameTable::EnsureExistsAndInsert(connection, anon::GetNormalizedNames(m_normalizer, manifest), manifestId);
        NormalizedPackagePublisherTable::EnsureExistsAndInsert(connection, anon::GetNormalizedPublishers(m_normalizer, manifest), manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::optional<std::filesystem::path>& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_2");

        auto [indexModified, manifestId] = V1_1::Interface::UpdateManifest(connection, manifest, relativePath);

        // Update new 1.2 tables as necessary
        indexModified = NormalizedPackageNameTable::UpdateIfNeededByManifestId(connection, anon::GetNormalizedNames(m_normalizer, manifest), manifestId) || indexModified;
        indexModified = NormalizedPackagePublisherTable::UpdateIfNeededByManifestId(connection, anon::GetNormalizedPublishers(m_normalizer, manifest), manifestId) || indexModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    void Interface::RemoveManifestById(SQLite::Connection& connection, SQLite::rowid_t manifestId)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "RemoveManifestById_v1_2");

        V1_1::Interface::RemoveManifestById(connection, manifestId);

        // Remove all of the new 1.2 data that is no longer referenced.
        NormalizedPackageNameTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        NormalizedPackagePublisherTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        savepoint.Commit();
    }

    bool Interface::CheckConsistency(const SQLite::Connection& connection, bool log) const
    {
        bool result = V1_1::Interface::CheckConsistency(connection, log);

        // If the v1.1 index was consistent, or if full logging of inconsistency was requested, check the v1.2 data.
        if (result || log)
        {
            result = NormalizedPackageNameTable::CheckConsistency(connection, log) && result;
        }

        if (result || log)
        {
            result = NormalizedPackagePublisherTable::CheckConsistency(connection, log) && result;
        }

        return result;
    }

    std::vector<std::string> Interface::GetMultiPropertyByPrimaryId(const SQLite::Connection& connection, SQLite::rowid_t primaryId, PackageVersionMultiProperty property) const
    {
        switch (property)
        {
            // These values are not right, as they are normalized.  But they are good enough for now and all we have.
        case PackageVersionMultiProperty::Name:
            return NormalizedPackageNameTable::GetValuesByManifestId(connection, primaryId);
        case PackageVersionMultiProperty::Publisher:
            return NormalizedPackagePublisherTable::GetValuesByManifestId(connection, primaryId);
        default:
            return V1_1::Interface::GetMultiPropertyByPrimaryId(connection, primaryId, property);
        }
    }

    Utility::NormalizedName Interface::NormalizeName(std::string_view name, std::string_view publisher) const
    {
        return m_normalizer.Normalize(name, publisher);
    }

    void Interface::DropTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "drop_tables_v1_2");

        V1_1::Interface::DropTables(connection);

        NormalizedPackageNameTable::Drop(connection);
        NormalizedPackagePublisherTable::Drop(connection);

        savepoint.Commit();
    }

    std::unique_ptr<V1_0::SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<V1_2::SearchResultsTable>(connection);
    }

    ISQLiteIndex::SearchResult Interface::SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const
    {
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

            return V1_1::Interface::SearchInternal(connection, request);
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
                result = V1_1::Interface::SearchInternal(connection, candidateSearch);
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

            return V1_1::Interface::SearchInternal(connection, request);
        }
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_2");

        V1_1::Interface::PrepareForPackaging(connection, false);

        NormalizedPackageNameTable::PrepareForPackaging(connection, GetOneToManyTableSchema(), true, true);
        NormalizedPackagePublisherTable::PrepareForPackaging(connection, GetOneToManyTableSchema(), true, true);

        savepoint.Commit();

        if (vacuum)
        {
            Vacuum(connection);
        }
    }
}
