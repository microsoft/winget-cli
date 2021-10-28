// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PackageTrackingCatalog.h"
#include "Microsoft/SQLiteIndexSource.h"
#include "AppInstallerDateTime.h"

using namespace std::string_literals;
using namespace AppInstaller::Repository::Microsoft;


namespace AppInstaller::Repository
{
    namespace
    {
        constexpr std::string_view c_PackageTrackingFileName = "installed.db";

        std::string CreateNameForCPRWL(const std::string& pathName)
        {
            return "PackageTrackingCPRWL_"s + pathName;
        }

        std::filesystem::path GetPackageTrackingFilePath(const std::string& pathName)
        {
            std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
            result /= pathName;
            result /= c_PackageTrackingFileName;
            return result;
        }
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

        std::string pathName = Utility::MakeSuitablePathPart(sourceIdentifier);

        std::string lockName = CreateNameForCPRWL(pathName);
        auto lock = Synchronization::CrossProcessReaderWriteLock::LockShared(lockName);

        std::filesystem::path trackingDB = GetPackageTrackingFilePath(pathName);

        if (!std::filesystem::exists(trackingDB))
        {
            lock.Release();
            lock = Synchronization::CrossProcessReaderWriteLock::LockExclusive(lockName);

            if (!std::filesystem::exists(trackingDB))
            {
                std::filesystem::create_directories(trackingDB.parent_path());
                SQLiteIndex::CreateNew(trackingDB.u8string(), Schema::Version::Latest(), SQLiteIndex::CreateOptions::SupportPathless);
            }

            lock.Release();
            lock = Synchronization::CrossProcessReaderWriteLock::LockShared(lockName);
        }

        SQLiteIndex index = SQLiteIndex::Open(trackingDB.u8string(), SQLiteIndex::OpenDisposition::ReadWrite);

        // TODO: Check schema version and upgrade as necessary when there is a relevant new schema.
        //       Could write this all now but it will be better tested when there is a new schema.

        // Create fake details for the source while stashing some information that might be helpful for debugging
        SourceDetails details;
        details.Identifier = "*Tracking";
        details.Name = "Tracking for "s + source.GetDetails().Name;
        details.Origin = SourceOrigin::PackageTracking;
        details.Arg = pathName;

        PackageTrackingCatalog result;
        result.m_implementation = std::make_shared<PackageTrackingCatalog::implementation>();
        result.m_implementation->Source = std::make_shared<SQLiteIndexSource>(details, std::move(index), std::move(lock));

        return result;
    }

    void PackageTrackingCatalog::RemoveForSource(const std::string& identifier)
    {
        if (identifier.empty())
        {
            THROW_HR(E_INVALIDARG);
        }

        std::string pathName = Utility::MakeSuitablePathPart(identifier);

        std::string lockName = CreateNameForCPRWL(pathName);
        auto lock = Synchronization::CrossProcessReaderWriteLock::LockExclusive(lockName);

        std::filesystem::path trackingDB = GetPackageTrackingFilePath(pathName);

        if (std::filesystem::exists(trackingDB))
        {
            std::filesystem::remove(trackingDB);
        }
    }

    SearchResult PackageTrackingCatalog::Search(const SearchRequest& request) const
    {
        return m_implementation->Source->Search(request);
    }

    struct PackageTrackingCatalog::Version::implementation
    {
        SQLiteIndex::IdType Id;
    };

    PackageTrackingCatalog::Version::Version() = default;
    PackageTrackingCatalog::Version::Version(const Version&) = default;
    PackageTrackingCatalog::Version& PackageTrackingCatalog::Version::operator=(const Version&) = default;
    PackageTrackingCatalog::Version::Version(Version&&) noexcept = default;
    PackageTrackingCatalog::Version& PackageTrackingCatalog::Version::operator=(Version&&) noexcept = default;
    PackageTrackingCatalog::Version::~Version() = default;

    PackageTrackingCatalog::Version::Version(std::shared_ptr<implementation>&& value) :
        m_implementation(std::move(value)) {}

    void PackageTrackingCatalog::Version::SetMetadata(PackageVersionMetadata metadata, const Utility::NormalizedString& value)
    {
        UNREFERENCED_PARAMETER(metadata);
        UNREFERENCED_PARAMETER(value);
        THROW_HR(E_NOTIMPL);
    }

    PackageTrackingCatalog::Version PackageTrackingCatalog::RecordInstall(
        const Manifest::Manifest& manifest,
        const Manifest::ManifestInstaller& installer,
        bool isUpgrade)
    {
        // TODO: Store additional information from these if needed
        UNREFERENCED_PARAMETER(installer);
        UNREFERENCED_PARAMETER(isUpgrade);

        auto& index = m_implementation->Source->GetIndex();

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

        std::shared_ptr<Version::implementation> result = std::make_shared<Version::implementation>();
        result->Id = manifestId;
        return { std::move(result) };
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
                auto manifestId = index.GetManifestIdByKey(match.first, version.GetVersion().ToString(), version.GetChannel().ToString());

                if (manifestId)
                {
                    index.RemoveManifestById(manifestId.value());
                }
            }
        }
    }
}
