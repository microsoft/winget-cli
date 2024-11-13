// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PackageTrackingCatalog.h"
#include "PackageTrackingCatalogSourceFactory.h"
#include "winget/Pin.h"
#include "winget/RepositorySource.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "AppInstallerDateTime.h"

using namespace std::string_literals;
using namespace AppInstaller::Repository::Microsoft;


namespace AppInstaller::Repository
{
    namespace
    {
        constexpr std::string_view c_PackageTrackingFileName = "installed.db";

        std::string CreateNameForCPL(const std::string& pathName)
        {
            return "PackageTrackingCPL_"s + pathName;
        }

        std::filesystem::path GetPackageTrackingFilePath(const std::string& pathName)
        {
            std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
            result /= pathName;
            result /= c_PackageTrackingFileName;
            return result;
        }

        // Call while holding the CrossProcessLock
        SQLiteIndex CreateOrOpenTrackingIndex(const std::filesystem::path& trackingDB)
        {
            if (!std::filesystem::exists(trackingDB))
            {
                std::filesystem::create_directories(trackingDB.parent_path());
                return SQLiteIndex::CreateNew(trackingDB.u8string(), SQLite::Version::Latest(), SQLiteIndex::CreateOptions::SupportPathless | SQLiteIndex::CreateOptions::DisableDependenciesSupport);
            }
            else
            {
                // TODO: Check schema version and upgrade as necessary when there is a relevant new schema.
                //       Could write this all now but it will be better tested when there is a new schema.
                return SQLiteIndex::Open(trackingDB.u8string(), SQLiteIndex::OpenDisposition::ReadWrite);
            }
        }

        struct PackageTrackingCatalogSourceReference : public ISourceReference
        {
            PackageTrackingCatalogSourceReference(const SourceDetails& details) : m_details(details) {}

            SourceDetails& GetDetails() override
            {
                return m_details;
            }

            std::string GetIdentifier() override
            {
                return m_details.Identifier;
            }

            std::shared_ptr<ISource> Open(IProgressCallback& callback) override
            {
                m_details.Arg = Utility::MakeSuitablePathPart(m_details.Data);
                std::filesystem::path trackingDB = GetPackageTrackingFilePath(m_details.Arg);

                Synchronization::CrossProcessLock lock(CreateNameForCPL(m_details.Arg));
                if (!lock.Acquire(callback))
                {
                    return {};
                }

                return std::make_shared<SQLiteIndexSource>(m_details, CreateOrOpenTrackingIndex(trackingDB));
            }

        private:
            // Store the identifier of the source in the Data field.
            SourceDetails m_details;
        };

        struct PackageTrackingCatalogSourceFactoryImpl : public ISourceFactory
        {
            std::string_view TypeName() const override final
            {
                return PackageTrackingCatalogSourceFactory::Type();
            }

            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, PackageTrackingCatalogSourceFactory::Type()));

                return std::make_shared<PackageTrackingCatalogSourceReference>(details);
            }

            bool Add(SourceDetails&, IProgressCallback&) override final
            {
                THROW_HR(E_NOTIMPL);
            }

            bool Update(const SourceDetails&, IProgressCallback&) override final
            {
                THROW_HR(E_NOTIMPL);
            }

            bool Remove(const SourceDetails& details, IProgressCallback& progress) override final
            {
                THROW_HR_IF(E_INVALIDARG, !Utility::CaseInsensitiveEquals(details.Type, PackageTrackingCatalogSourceFactory::Type()));

                std::string pathName = Utility::MakeSuitablePathPart(details.Data);

                Synchronization::CrossProcessLock lock(CreateNameForCPL(pathName));
                if (!lock.Acquire(progress))
                {
                    return false;
                }

                std::filesystem::path trackingDB = GetPackageTrackingFilePath(pathName);

                if (std::filesystem::exists(trackingDB))
                {
                    std::filesystem::remove(trackingDB);
                }

                return true;
            }
        };
    }

    struct PackageTrackingCatalog::implementation
    {
        std::shared_ptr<Microsoft::SQLiteIndexSource> Source;
    };

    PackageTrackingCatalog::PackageTrackingCatalog() = default;
    PackageTrackingCatalog::PackageTrackingCatalog(const PackageTrackingCatalog&) = default;
    PackageTrackingCatalog& PackageTrackingCatalog::operator=(const PackageTrackingCatalog&) = default;
    PackageTrackingCatalog::PackageTrackingCatalog(PackageTrackingCatalog&&) noexcept = default;
    PackageTrackingCatalog& PackageTrackingCatalog::operator=(PackageTrackingCatalog&&) noexcept = default;
    PackageTrackingCatalog::~PackageTrackingCatalog() = default;

    PackageTrackingCatalog PackageTrackingCatalog::CreateForSource(const Source& source)
    {
        // Not a valid source for tracking
        const std::string sourceIdentifier = source.GetIdentifier();
        if (sourceIdentifier.empty() || !source.ContainsAvailablePackages())
        {
            THROW_HR(E_INVALIDARG);
        }

        // Create fake details for the source while stashing some information that might be helpful for debugging
        SourceDetails details;
        details.Type = PackageTrackingCatalogSourceFactory::Type();
        details.Identifier = "*Tracking";
        details.Name = "Tracking for "s + source.GetDetails().Name;
        details.Origin = SourceOrigin::PackageTracking;
        details.Data = sourceIdentifier;

        ProgressCallback dummyProgress;

        PackageTrackingCatalog result;
        result.m_implementation = std::make_shared<PackageTrackingCatalog::implementation>();
        result.m_implementation->Source = SourceCast<SQLiteIndexSource>(ISourceFactory::GetForType(details.Type)->Create(details)->Open(dummyProgress));

        return result;
    }

    void PackageTrackingCatalog::RemoveForSource(const std::string& identifier)
    {
        if (identifier.empty())
        {
            THROW_HR(E_INVALIDARG);
        }

        // Create details to pass to the factory; the identifier of the source is passed in the Data field.
        SourceDetails dummyDetails;
        dummyDetails.Type = PackageTrackingCatalogSourceFactory::Type();
        dummyDetails.Data = identifier;

        ProgressCallback dummyProgress;

        ISourceFactory::GetForType(dummyDetails.Type)->Remove(dummyDetails, dummyProgress);
    }

    PackageTrackingCatalog::operator bool() const
    {
        return static_cast<bool>(m_implementation);
    }

    SearchResult PackageTrackingCatalog::Search(const SearchRequest& request) const
    {
        return m_implementation->Source->Search(request);
    }

    struct PackageTrackingCatalog::Version::implementation
    {
        SQLiteIndex::IdType Id;
    };

    PackageTrackingCatalog::Version::Version(const Version&) = default;
    PackageTrackingCatalog::Version& PackageTrackingCatalog::Version::operator=(const Version&) = default;
    PackageTrackingCatalog::Version::Version(Version&&) noexcept = default;
    PackageTrackingCatalog::Version& PackageTrackingCatalog::Version::operator=(Version&&) noexcept = default;
    PackageTrackingCatalog::Version::~Version() = default;

    PackageTrackingCatalog::Version::Version(PackageTrackingCatalog& catalog, std::shared_ptr<implementation>&& value) :
        m_catalog(catalog), m_implementation(std::move(value)) {}

    void PackageTrackingCatalog::Version::SetMetadata(PackageVersionMetadata metadata, const Utility::NormalizedString& value)
    {
        auto& index = m_catalog.m_implementation->Source->GetIndex();
        index.SetMetadataByManifestId(m_implementation->Id, metadata, value);
    }

    PackageTrackingCatalog::Version PackageTrackingCatalog::RecordInstall(
        Manifest::Manifest& manifest,
        const Manifest::ManifestInstaller& installer,
        bool isUpgrade)
    {
        // TODO: Store additional information from these if needed
        UNREFERENCED_PARAMETER(isUpgrade);

        auto& index = m_implementation->Source->GetIndex();

        // Strip ARP version information from the manifest if it is present
        for (auto& arpRangeRemovedInstaller : manifest.Installers)
        {
            for (auto& arpRangeRemovedEntry : arpRangeRemovedInstaller.AppsAndFeaturesEntries)
            {
                arpRangeRemovedEntry.DisplayVersion.clear();
            }
        }

        // Check for an existing manifest that matches this one (could be reinstalling)
        auto manifestIdOpt = index.GetManifestIdByManifest(manifest);

        if (manifestIdOpt)
        {
            index.UpdateManifest(manifest);
        }
        else
        {
            manifestIdOpt = index.AddManifest(manifest);
        }

        SQLiteIndex::IdType manifestId = manifestIdOpt.value();

        // Write additional metadata for package tracking
        std::ostringstream strstr;
        strstr << Utility::GetCurrentUnixEpoch();
        index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::TrackingWriteTime, strstr.str());

        if (installer.RequireExplicitUpgrade)
        {
            index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::PinnedState, ToString(Pinning::PinType::PinnedByManifest));
        }

        // Record installed architecture and locale if applicable
        index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledArchitecture, ToString(installer.Arch));
        if (!installer.Locale.empty())
        {
            index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledLocale, installer.Locale);
        }

        std::shared_ptr<Version::implementation> result = std::make_shared<Version::implementation>();
        result->Id = manifestId;
        return { *this, std::move(result) };
    }

    void PackageTrackingCatalog::RecordUninstall(const Utility::LocIndString& packageIdentifier)
    {
        auto& index = m_implementation->Source->GetIndex();

        SearchRequest idSearch;
        idSearch.Filters.emplace_back(PackageMatchField::Id, MatchType::CaseInsensitive, packageIdentifier.get());
        auto searchResult = index.Search(idSearch);

        for (const auto& match : searchResult.Matches)
        {
            auto versions = index.GetVersionKeysById(match.first);

            for (const auto& version : versions)
            {
                index.RemoveManifestById(version.ManifestId);
            }
        }
    }

    std::unique_ptr<ISourceFactory> PackageTrackingCatalogSourceFactory::Create()
    {
        return std::make_unique<PackageTrackingCatalogSourceFactoryImpl>();
    }
}
