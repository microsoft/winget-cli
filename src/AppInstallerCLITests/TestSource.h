// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <ISource.h>
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

        TestPackageVersion(const Manifest& manifest, std::weak_ptr<const ISource> source = {}, bool hideSystemReferenceStrings = false);
        TestPackageVersion(const Manifest& manifest, MetadataMap installationMetadata, std::weak_ptr<const ISource> source = {});

        template <typename... Args>
        static std::shared_ptr<TestPackageVersion> Make(Args&&... args)
        {
            return std::make_shared<TestPackageVersion>(std::forward<Args>(args)...);
        }

        LocIndString GetProperty(AppInstaller::Repository::PackageVersionProperty property) const override;
        std::vector<LocIndString> GetMultiProperty(AppInstaller::Repository::PackageVersionMultiProperty property) const override;
        Manifest GetManifest() override;
        AppInstaller::Repository::Source GetSource() const override;
        MetadataMap GetMetadata() const override;

        Manifest VersionManifest;
        MetadataMap Metadata;
        std::weak_ptr<const ISource> Source;
        bool HideSystemReferenceStrings = false;

    protected:
        static void AddIfHasValueAndNotPresent(const AppInstaller::Utility::NormalizedString& value, std::vector<LocIndString>& target, bool folded = false);
    };

    // IPackage for TestSource
    struct TestPackage : public AppInstaller::Repository::IPackage
    {
        static constexpr AppInstaller::Repository::IPackageType PackageType = AppInstaller::Repository::IPackageType::TestPackage;

        using Manifest = AppInstaller::Manifest::Manifest;
        using ISource = AppInstaller::Repository::ISource;
        using LocIndString = AppInstaller::Utility::LocIndString;
        using MetadataMap = TestPackageVersion::MetadataMap;

        // Create a package with only available versions using these manifests.
        TestPackage(const std::vector<Manifest>& available, std::weak_ptr<const ISource> source = {}, bool hideSystemReferenceStringsOnVersion = false);

        // Create a package with an installed version, metadata, and optionally available versions.
        TestPackage(const Manifest& installed, MetadataMap installationMetadata, std::weak_ptr<const ISource> source = {});

        template <typename... Args>
        static std::shared_ptr<TestPackage> Make(Args&&... args)
        {
            return std::make_shared<TestPackage>(std::forward<Args>(args)...);
        }

        AppInstaller::Utility::LocIndString GetProperty(AppInstaller::Repository::PackageProperty property) const override;
        std::vector<AppInstaller::Repository::PackageVersionKey> GetVersionKeys() const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetLatestVersion() const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetVersion(const AppInstaller::Repository::PackageVersionKey& versionKey) const override;
        AppInstaller::Repository::Source GetSource() const override;
        bool IsSame(const IPackage* other) const override;
        const void* CastTo(AppInstaller::Repository::IPackageType type) const override;

        std::vector<std::shared_ptr<AppInstaller::Repository::IPackageVersion>> Versions;
        std::weak_ptr<const ISource> Source;
        size_t DefaultIsSameIdentity = 0;
        std::function<bool(const IPackage*, const IPackage*)> IsSameOverride;
    };

    // ICompositePackage for TestSource
    struct TestCompositePackage : public AppInstaller::Repository::ICompositePackage
    {
        using Manifest = AppInstaller::Manifest::Manifest;
        using ISource = AppInstaller::Repository::ISource;
        using LocIndString = AppInstaller::Utility::LocIndString;
        using MetadataMap = TestPackageVersion::MetadataMap;

        // Create a package with only available versions using these manifests.
        TestCompositePackage(const std::vector<Manifest>& available, std::weak_ptr<const ISource> source = {}, bool hideSystemReferenceStringsOnVersion = false);

        // Create a package with an installed version, metadata, and optionally available versions.
        TestCompositePackage(const Manifest& installed, MetadataMap installationMetadata, const std::vector<Manifest>& available = {}, std::weak_ptr<const ISource> source = {});

        template <typename... Args>
        static std::shared_ptr<TestCompositePackage> Make(Args&&... args)
        {
            return std::make_shared<TestCompositePackage>(std::forward<Args>(args)...);
        }

        AppInstaller::Utility::LocIndString GetProperty(AppInstaller::Repository::PackageProperty property) const override;
        std::shared_ptr<AppInstaller::Repository::IPackage> GetInstalled() override;
        std::vector<std::shared_ptr<AppInstaller::Repository::IPackage>> GetAvailable() override;

        std::shared_ptr<TestPackage> Installed;
        std::vector<std::shared_ptr<TestPackage>> Available;
    };

    // An ISource implementation for use across the test code.
    struct TestSource : public AppInstaller::Repository::ISource, public std::enable_shared_from_this<TestSource>
    {
        static constexpr AppInstaller::Repository::ISourceType SourceType = AppInstaller::Repository::ISourceType::TestSource;

        const AppInstaller::Repository::SourceDetails& GetDetails() const override;
        const std::string& GetIdentifier() const override;
        AppInstaller::Repository::SourceInformation GetInformation() const override;

        bool QueryFeatureFlag(AppInstaller::Repository::SourceFeatureFlag flag) const override;
        std::function<bool(AppInstaller::Repository::SourceFeatureFlag)> QueryFeatureFlagFunction;

        AppInstaller::Repository::SearchResult Search(const AppInstaller::Repository::SearchRequest& request) const override;
        void* CastTo(AppInstaller::Repository::ISourceType type) override;

        AppInstaller::Repository::SourceDetails Details = { "TestSource", "Microsoft.TestSource", "//arg", "", "*TestSource" };
        AppInstaller::Repository::SourceInformation Information;
        std::function<AppInstaller::Repository::SearchResult(const AppInstaller::Repository::SearchRequest& request)> SearchFunction;

        TestSource() = default;
        TestSource(const AppInstaller::Repository::SourceDetails& details) : Details(details) {}
    };

    struct TestSourceReference : public AppInstaller::Repository::ISourceReference
    {
        using OpenFunctor = std::function<std::shared_ptr<AppInstaller::Repository::ISource>(const AppInstaller::Repository::SourceDetails&)>;
        using OpenFunctorWithCustomHeader = std::function<std::shared_ptr<AppInstaller::Repository::ISource>(const AppInstaller::Repository::SourceDetails&, std::optional<std::string>)>;

        TestSourceReference(const AppInstaller::Repository::SourceDetails& details, OpenFunctor open) : m_details(details), m_onOpen(open) {}
        TestSourceReference(const AppInstaller::Repository::SourceDetails& details, OpenFunctorWithCustomHeader open) : m_details(details), m_onOpenWithCustomHeader(open) {}

        std::string GetIdentifier() override { return m_details.Identifier; }

        AppInstaller::Repository::SourceDetails& GetDetails() override { return m_details; };

        bool SetCustomHeader(std::optional<std::string> header) override { m_header = header; return true; }

        bool ShouldUpdateBeforeOpenResult = false;
        bool ShouldUpdateBeforeOpen(const std::optional<AppInstaller::Repository::TimeSpan>&) override { return ShouldUpdateBeforeOpenResult; }

        std::shared_ptr<AppInstaller::Repository::ISource> Open(AppInstaller::IProgressCallback&) override
        {
            if (m_onOpenWithCustomHeader)
            {
                return m_onOpenWithCustomHeader(m_details, m_header);
            }
            else
            {
                return m_onOpen(m_details);
            }
        }

    private:
        AppInstaller::Repository::SourceDetails m_details;
        OpenFunctor m_onOpen;
        OpenFunctorWithCustomHeader m_onOpenWithCustomHeader;
        std::optional<std::string> m_header;
    };

    // An ISourceFactory implementation for use across the test code.
    struct TestSourceFactory : public AppInstaller::Repository::ISourceFactory
    {
        using OpenFunctor = std::function<std::shared_ptr<AppInstaller::Repository::ISource>(const AppInstaller::Repository::SourceDetails&)>;
        using OpenFunctorWithCustomHeader = std::function<std::shared_ptr<AppInstaller::Repository::ISource>(const AppInstaller::Repository::SourceDetails&, std::optional<std::string>)>;
        using AddFunctor = std::function<void(AppInstaller::Repository::SourceDetails&)>;
        using UpdateFunctor = std::function<void(const AppInstaller::Repository::SourceDetails&)>;
        using RemoveFunctor = std::function<void(const AppInstaller::Repository::SourceDetails&)>;

        TestSourceFactory(OpenFunctor open) : OnOpen(std::move(open)) {}
        TestSourceFactory(OpenFunctorWithCustomHeader open) : OnOpenWithCustomHeader(std::move(open)) {}

        // ISourceFactory
        std::string_view TypeName() const override;
        std::shared_ptr<AppInstaller::Repository::ISourceReference> Create(const AppInstaller::Repository::SourceDetails& details) override;
        bool Add(AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;
        bool Update(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;
        bool Remove(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback&) override;

        // Make copies of self when requested.
        operator std::function<std::unique_ptr<AppInstaller::Repository::ISourceFactory>()>();

        bool ShouldUpdateBeforeOpenResult = false;
        OpenFunctor OnOpen;
        OpenFunctorWithCustomHeader OnOpenWithCustomHeader;
        AddFunctor OnAdd;
        UpdateFunctor OnUpdate;
        RemoveFunctor OnRemove;
    };

    bool AddSource(const AppInstaller::Repository::SourceDetails& details, AppInstaller::IProgressCallback& progress);
    bool UpdateSource(std::string_view name, AppInstaller::IProgressCallback& progress);
    bool RemoveSource(std::string_view name, AppInstaller::IProgressCallback& progress);
    AppInstaller::Repository::Source OpenSource(std::string_view name, AppInstaller::IProgressCallback& progress);
    void DropSource(std::string_view name);
    std::vector<AppInstaller::Repository::SourceDetails> GetSources();
}
