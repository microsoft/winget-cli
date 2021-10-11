// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/PackageTrackingCatalog.h"
#include "Microsoft/SQLiteIndexSource.h"


namespace AppInstaller::Repository
{
    namespace
    {
        constexpr std::string_view c_PackageTrackingDirectoryName = "";

        std::filesystem::path GetPackageTrackingRootPath()
        {
            std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
            result /= 
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

    PackageTrackingCatalog PackageTrackingCatalog::CreateForSource(const std::shared_ptr<const ISource>& source)
    {
        THROW_HR_IF(E_INVALIDARG, details.Type != PreIndexedPackageSourceFactory::Type());

        auto lock = Synchronization::CrossProcessReaderWriteLock::LockShared(CreateNameForCPRWL(details), progress);
        if (!lock)
        {
            return {};
        }

        std::filesystem::path packageLocation = GetStatePathFromDetails(details);
        packageLocation /= s_PreIndexedPackageSourceFactory_IndexFileName;

        if (!std::filesystem::exists(packageLocation))
        {
            AICLI_LOG(Repo, Info, << "Data not found at " << packageLocation);
            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
        }

        SQLiteIndex index = SQLiteIndex::Open(packageLocation.u8string(), SQLiteIndex::OpenDisposition::Read);

        // We didn't use to store the source identifier, so we compute it here in case it's
        // missing from the details.
        return std::make_shared<SQLiteIndexSource>(details, GetPackageFamilyNameFromDetails(details), std::move(index), std::move(lock));
    }

    SearchResult PackageTrackingCatalog::Search(const SearchRequest& request) const
    {
        return m_implementation->Source->Search(request);
    }

    struct PackageTrackingCatalog::Version::implementation
    {
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
        UNREFERENCED_PARAMETER(manifest);
        UNREFERENCED_PARAMETER(installer);
        UNREFERENCED_PARAMETER(isUpgrade);
        THROW_HR(E_NOTIMPL);
    }

    void PackageTrackingCatalog::RecordUninstall(const Utility::LocIndString& packageIdentifier)
    {
        UNREFERENCED_PARAMETER(packageIdentifier);
        THROW_HR(E_NOTIMPL);
    }
}
