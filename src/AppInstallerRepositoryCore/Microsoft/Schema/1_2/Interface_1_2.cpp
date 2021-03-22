// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_2/Interface.h"

#include "Microsoft/Schema/1_2/NormalizedPackageNameTable.h"
#include "Microsoft/Schema/1_2/NormalizedPackagePublisherTable.h"

#include "Microsoft/Schema/1_2/SearchResultsTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_2
{
    namespace
    {
        void AddLocalizationNormalizedName(const Utility::NameNormalizer& normalizer, const Manifest::ManifestLocalization& localization, std::vector<Utility::NormalizedString>& out)
        {
            if (localization.Contains(Manifest::Localization::PackageName))
            {
                Utility::NormalizedString value = normalizer.NormalizeName(Utility::FoldCase(localization.Get<Manifest::Localization::PackageName>())).Name();
                if (std::find(out.begin(), out.end(), value) == out.end())
                {
                    out.emplace_back(std::move(value));
                }
            }
        }

        void AddLocalizationNormalizedPublisher(const Utility::NameNormalizer& normalizer, const Manifest::ManifestLocalization& localization, std::vector<Utility::NormalizedString>& out)
        {
            if (localization.Contains(Manifest::Localization::Publisher))
            {
                Utility::NormalizedString value = normalizer.NormalizePublisher(Utility::FoldCase(localization.Get<Manifest::Localization::Publisher>()));
                if (std::find(out.begin(), out.end(), value) == out.end())
                {
                    out.emplace_back(std::move(value));
                }
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

            return result;
        }
    }

    Interface::Interface(Utility::NormalizationVersion normVersion) : m_normalizer(normVersion)
    {
    }

    Schema::Version Interface::GetVersion() const
    {
        return { 1, 2 };
    }

    void Interface::CreateTables(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createtables_v1_2");

        V1_1::Interface::CreateTables(connection);

        // While the name and publisher should be linked per-locale, we are not implementing that here.
        // This will mean that one can match cross locale name and publisher, but the chance that this
        // leads to a confusion between packages is very small. More likely would be intentional attempts
        // to confuse the correlation, which could be fairly easily carried out even with linked values.
        NormalizedPackageNameTable::Create(connection);
        NormalizedPackagePublisherTable::Create(connection);

        savepoint.Commit();
    }

    SQLite::rowid_t Interface::AddManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addmanifest_v1_2");

        SQLite::rowid_t manifestId = V1_1::Interface::AddManifest(connection, manifest, relativePath);

        // Add the new 1.2 data
        // These normalized strings are all stored with their cases folded so that they can be
        // looked up ordinally; enabling the index to provide efficient searches.
        NormalizedPackageNameTable::EnsureExistsAndInsert(connection, GetNormalizedNames(m_normalizer, manifest), manifestId);
        NormalizedPackagePublisherTable::EnsureExistsAndInsert(connection, GetNormalizedPublishers(m_normalizer, manifest), manifestId);

        savepoint.Commit();

        return manifestId;
    }

    std::pair<bool, SQLite::rowid_t> Interface::UpdateManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "updatemanifest_v1_2");

        auto [indexModified, manifestId] = V1_1::Interface::UpdateManifest(connection, manifest, relativePath);

        // Update new 1.2 tables as necessary
        indexModified = NormalizedPackageNameTable::UpdateIfNeededByManifestId(connection, GetNormalizedNames(m_normalizer, manifest), manifestId) || indexModified;
        indexModified = NormalizedPackagePublisherTable::UpdateIfNeededByManifestId(connection, GetNormalizedPublishers(m_normalizer, manifest), manifestId) || indexModified;

        savepoint.Commit();

        return { indexModified, manifestId };
    }

    SQLite::rowid_t Interface::RemoveManifest(SQLite::Connection& connection, const Manifest::Manifest& manifest, const std::filesystem::path& relativePath)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "removemanifest_v1_2");

        SQLite::rowid_t manifestId = V1_1::Interface::RemoveManifest(connection, manifest, relativePath);

        // Remove all of the new 1.2 data that is no longer referenced.
        NormalizedPackageNameTable::DeleteIfNotNeededByManifestId(connection, manifestId);
        NormalizedPackagePublisherTable::DeleteIfNotNeededByManifestId(connection, manifestId);

        savepoint.Commit();

        return manifestId;
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

    Utility::NormalizedName Interface::NormalizeName(std::string_view name, std::string_view publisher) const
    {
        return m_normalizer.Normalize(name, publisher);
    }

    std::unique_ptr<V1_0::SearchResultsTable> Interface::CreateSearchResultsTable(const SQLite::Connection& connection) const
    {
        return std::make_unique<SearchResultsTable>(connection);
    }

    ISQLiteIndex::SearchResult Interface::SearchInternal(const SQLite::Connection& connection, SearchRequest& request) const
    {
        // Update NormalizedNameAndPublisher with normalization and folding
        auto updateIfNeeded = [&](PackageMatchFilter& filter)
        {
            if (filter.Field == PackageMatchField::NormalizedNameAndPublisher && filter.Type == MatchType::Exact)
            {
                Utility::NormalizedName normalized = m_normalizer.Normalize(Utility::FoldCase(filter.Value), Utility::FoldCase(filter.Additional.value()));
                filter.Value = normalized.Name();
                filter.Additional = normalized.Publisher();
            }
        };

        for (auto& inclusion : request.Inclusions)
        {
            updateIfNeeded(inclusion);
        }

        for (auto& filter : request.Filters)
        {
            updateIfNeeded(filter);
        }

        return V1_1::Interface::SearchInternal(connection, request);
    }

    void Interface::PrepareForPackaging(SQLite::Connection& connection, bool vacuum)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "prepareforpackaging_v1_2");

        V1_1::Interface::PrepareForPackaging(connection, false);

        NormalizedPackageNameTable::PrepareForPackaging(connection, true, true);
        NormalizedPackagePublisherTable::PrepareForPackaging(connection, true, true);

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
