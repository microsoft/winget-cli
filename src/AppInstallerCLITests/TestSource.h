// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySource.h>
#include <winget/Manifest.h>

#include <functional>
#include <utility>

namespace TestCommon
{
    // IPackageVersion for TestSource
    struct TestPackageVersion : public AppInstaller::Repository::IPackageVersion
    {
        using Manifest = AppInstaller::Manifest::Manifest;
        using ISource = AppInstaller::Repository::ISource;
        using LocIndString = AppInstaller::Utility::LocIndString;
        using MetadataMap = AppInstaller::Repository::IPackageVersion::Metadata;

        TestPackageVersion(const Manifest& manifest, MetadataMap installationMetadata = {});

        template <typename... Args>
        static std::shared_ptr<TestPackageVersion> Make(Args&&... args)
        {
            return std::make_shared<TestPackageVersion>(std::forward<Args>(args)...);
        }

        LocIndString GetProperty(AppInstaller::Repository::PackageVersionProperty property) const override;
        std::vector<LocIndString> GetMultiProperty(AppInstaller::Repository::PackageVersionMultiProperty property) const override;
        Manifest GetManifest() const override;
        std::shared_ptr<const ISource> GetSource() const override;
        MetadataMap GetMetadata() const override;

        Manifest VersionManifest;
        MetadataMap Metadata;
        std::shared_ptr<const ISource> Source;

    protected:
        static void AddFoldedIfHasValueAndNotPresent(const AppInstaller::Utility::NormalizedString& value, std::vector<LocIndString>& target);
    };

    // IPackage for TestSource
    struct TestPackage : public AppInstaller::Repository::IPackage
    {
        using Manifest = AppInstaller::Manifest::Manifest;
        using LocIndString = AppInstaller::Utility::LocIndString;
        using MetadataMap = TestPackageVersion::MetadataMap;

        // Create a package with only available versions using these manifests.
        TestPackage(const std::vector<Manifest>& available);

        // Create a package with an installed version, metadata, and optionally available versions.
        TestPackage(const Manifest& installed, MetadataMap installationMetadata, const std::vector<Manifest>& available = {});

        template <typename... Args>
        static std::shared_ptr<TestPackage> Make(Args&&... args)
        {
            return std::make_shared<TestPackage>(std::forward<Args>(args)...);
        }

        AppInstaller::Utility::LocIndString GetProperty(AppInstaller::Repository::PackageProperty property) const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetInstalledVersion() const override;
        std::vector<AppInstaller::Repository::PackageVersionKey> GetAvailableVersionKeys() const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetLatestAvailableVersion() const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetAvailableVersion(const AppInstaller::Repository::PackageVersionKey& versionKey) const override;
        bool IsUpdateAvailable() const override;

        std::shared_ptr<AppInstaller::Repository::IPackageVersion> InstalledVersion;
        std::vector<std::shared_ptr<AppInstaller::Repository::IPackageVersion>> AvailableVersions;
    };

    // An ISource implementation for use across the test code.
    struct TestSource : public AppInstaller::Repository::ISource
    {
        const AppInstaller::Repository::SourceDetails& GetDetails() const override;
        const std::string& GetIdentifier() const override;
        AppInstaller::Repository::SearchResult Search(const AppInstaller::Repository::SearchRequest& request) const override;
        bool IsComposite() const override;

        AppInstaller::Repository::SourceDetails Details;
        std::string Identifier = "*TestSource";
        std::function<AppInstaller::Repository::SearchResult(const AppInstaller::Repository::SearchRequest& request)> SearchFunction;
        bool Composite = false;
    };
}
