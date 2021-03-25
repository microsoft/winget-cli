// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySource.h>
#include <winget/Manifest.h>
#include <SourceFactory.h>

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

        TestPackageVersion(const Manifest& manifest, std::weak_ptr<const ISource> source = {});
        TestPackageVersion(const Manifest& manifest, MetadataMap installationMetadata, std::weak_ptr<const ISource> source = {});

        template <typename... Args>
        static std::shared_ptr<TestPackageVersion> Make(Args&&... args)
        {
            return std::make_shared<TestPackageVersion>(std::forward<Args>(args)...);
        }

        LocIndString GetProperty(AppInstaller::Repository::PackageVersionProperty property) const override;
        std::vector<LocIndString> GetMultiProperty(AppInstaller::Repository::PackageVersionMultiProperty property) const override;
        Manifest GetManifest() override;
        std::shared_ptr<const ISource> GetSource() const override;
        MetadataMap GetMetadata() const override;

        Manifest VersionManifest;
        MetadataMap Metadata;
        std::weak_ptr<const ISource> Source;

    protected:
        static void AddFoldedIfHasValueAndNotPresent(const AppInstaller::Utility::NormalizedString& value, std::vector<LocIndString>& target);
    };

    // IPackage for TestSource
    struct TestPackage : public AppInstaller::Repository::IPackage
    {
        using Manifest = AppInstaller::Manifest::Manifest;
        using ISource = AppInstaller::Repository::ISource;
        using LocIndString = AppInstaller::Utility::LocIndString;
        using MetadataMap = TestPackageVersion::MetadataMap;

        // Create a package with only available versions using these manifests.
        TestPackage(const std::vector<Manifest>& available, std::weak_ptr<const ISource> source = {});

        // Create a package with an installed version, metadata, and optionally available versions.
        TestPackage(const Manifest& installed, MetadataMap installationMetadata, const std::vector<Manifest>& available = {}, std::weak_ptr<const ISource> source = {});

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
        bool IsSame(const IPackage* other) const override;

        std::shared_ptr<AppInstaller::Repository::IPackageVersion> InstalledVersion;
        std::vector<std::shared_ptr<AppInstaller::Repository::IPackageVersion>> AvailableVersions;
    };

    // An ISource implementation for use across the test code.
    struct TestSource : public AppInstaller::Repository::ISource, public std::enable_shared_from_this<TestSource>
    {
        const AppInstaller::Repository::SourceDetails& GetDetails() const override;
        const std::string& GetIdentifier() const override;
        AppInstaller::Repository::SearchResult Search(const AppInstaller::Repository::SearchRequest& request) const override;
        bool IsComposite() const override;

        AppInstaller::Repository::SourceDetails Details = { "TestSource", "Microsoft.TestSource", "//arg", "", "*TestSource" };
        std::function<AppInstaller::Repository::SearchResult(const AppInstaller::Repository::SearchRequest& request)> SearchFunction;
        bool Composite = false;
    };

    // An ISourceFactory implementation for use across the test code.
    struct TestSourceFactory : public AppInstaller::Repository::ISourceFactory
    {
        using CreateFunctor = std::function<std::shared_ptr<AppInstaller::Repository::ISource>(const AppInstaller::Repository::SourceDetails&)>;
        using AddFunctor = std::function<void(AppInstaller::Repository::SourceDetails&)>;
        using UpdateFunctor = std::function<void(const AppInstaller::Repository::SourceDetails&)>;
        using RemoveFunctor = std::function<void(const AppInstaller::Repository::SourceDetails&)>;

        TestSourceFactory(CreateFunctor create) : OnCreate(std::move(create)) {}

        // ISourceFactory
        std::shared_ptr<AppInstaller::Repository::ISource> Create(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;
        void Add(AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;
        void Update(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;
        void Remove(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;

        // Make copies of self when requested.
        operator std::function<std::unique_ptr<AppInstaller::Repository::ISourceFactory>()>();

        CreateFunctor OnCreate;
        AddFunctor OnAdd;
        UpdateFunctor OnUpdate;
        RemoveFunctor OnRemove;
    };
}
