// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "CatalogPackage.h"
#include "PackageVersionInfo.h"

namespace Deployment = winrt::Microsoft::Management::Deployment;
namespace Implementation = Deployment::implementation;
namespace Collections = winrt::Windows::Foundation::Collections;

namespace
{
    struct ForeignPackageVersionInfo : winrt::implements<ForeignPackageVersionInfo, Deployment::IPackageVersionInfo>
    {
        winrt::hstring GetMetadata(Deployment::PackageVersionMetadataField) { return {}; }
        winrt::hstring Id() { return {}; }
        winrt::hstring DisplayName() { return {}; }
        winrt::hstring Version() { return {}; }
        winrt::hstring Channel() { return {}; }
        Collections::IVectorView<winrt::hstring> PackageFamilyNames() { return nullptr; }
        Collections::IVectorView<winrt::hstring> ProductCodes() { return nullptr; }
        Deployment::PackageCatalog PackageCatalog() { return nullptr; }
    };

    struct ForeignCatalogPackage : winrt::implements<ForeignCatalogPackage, Deployment::ICatalogPackage>
    {
        ForeignCatalogPackage(Deployment::PackageVersionInfo version) : m_version(std::move(version)) {}

        winrt::hstring Id() { return {}; }
        winrt::hstring Name() { return {}; }
        Deployment::PackageVersionInfo InstalledVersion() { return m_version; }
        Collections::IVectorView<Deployment::PackageVersionId> AvailableVersions() { return nullptr; }
        Deployment::PackageVersionInfo DefaultInstallVersion() { return m_version; }
        Deployment::PackageVersionInfo GetPackageVersionInfo(Deployment::PackageVersionId const&) { return m_version; }
        bool IsUpdateAvailable() { return false; }

    private:
        Deployment::PackageVersionInfo m_version;
    };
}

TEST_CASE("PackageObjectAccess_RejectsForeignVersions", "[COM]")
{
    auto foreignVersion = winrt::make<ForeignPackageVersionInfo>().as<Deployment::PackageVersionInfo>();

    REQUIRE_THROWS_HR(Implementation::PackageVersionInfo::GetRepositoryPackageVersion(foreignVersion), E_INVALIDARG);
    REQUIRE_THROWS_HR(Implementation::PackageVersionInfo::GetRepositoryPackageVersion(Deployment::PackageVersionInfo{ nullptr }), E_INVALIDARG);
}

TEST_CASE("PackageObjectAccess_RetainsLocalVersion", "[COM]")
{
    auto repositoryVersion = TestCommon::TestPackageVersion::Make(AppInstaller::Manifest::Manifest{});
    auto localVersion = winrt::make_self<Implementation::PackageVersionInfo>();
    localVersion->Initialize(repositoryVersion);

    auto version = localVersion.as<Deployment::PackageVersionInfo>();
    auto result = Implementation::PackageVersionInfo::GetRepositoryPackageVersion(version);
    REQUIRE(result == repositoryVersion);

    std::weak_ptr<AppInstaller::Repository::IPackageVersion> weakVersion = repositoryVersion;
    repositoryVersion.reset();
    localVersion = nullptr;
    version = nullptr;
    REQUIRE_FALSE(weakVersion.expired());
    result.reset();
    REQUIRE(weakVersion.expired());
}

TEST_CASE("PackageObjectAccess_RejectsForeignPackagesWithLocalVersions", "[COM]")
{
    auto repositoryVersion = TestCommon::TestPackageVersion::Make(AppInstaller::Manifest::Manifest{});
    auto localVersion = winrt::make_self<Implementation::PackageVersionInfo>();
    localVersion->Initialize(repositoryVersion);

    auto foreignPackage = winrt::make<ForeignCatalogPackage>(localVersion.as<Deployment::PackageVersionInfo>()).as<Deployment::CatalogPackage>();
    REQUIRE(Implementation::PackageVersionInfo::GetRepositoryPackageVersion(foreignPackage.InstalledVersion()) == repositoryVersion);
    REQUIRE_THROWS_HR(Implementation::CatalogPackage::GetRepositoryPackage(foreignPackage), E_INVALIDARG);
    REQUIRE_THROWS_HR(Implementation::CatalogPackage::GetRepositoryPackage(Deployment::CatalogPackage{ nullptr }), E_INVALIDARG);
}

TEST_CASE("PackageObjectAccess_RetainsLocalPackage", "[COM]")
{
    auto repositoryPackage = TestCommon::TestCompositePackage::Make(std::vector<AppInstaller::Manifest::Manifest>{});
    auto localPackage = winrt::make_self<Implementation::CatalogPackage>();
    localPackage->Initialize({}, repositoryPackage);

    auto package = localPackage.as<Deployment::CatalogPackage>();
    auto result = Implementation::CatalogPackage::GetRepositoryPackage(package);
    REQUIRE(result == repositoryPackage);

    std::weak_ptr<AppInstaller::Repository::ICompositePackage> weakPackage = repositoryPackage;
    repositoryPackage.reset();
    localPackage = nullptr;
    package = nullptr;
    REQUIRE_FALSE(weakPackage.expired());
    result.reset();
    REQUIRE(weakPackage.expired());
}
