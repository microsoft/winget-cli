// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySource.h>

#include <functional>

namespace TestCommon
{
    using namespace AppInstaller::Manifest;
    using namespace AppInstaller::Repository;
    using namespace AppInstaller::Utility;

    // IPackageVersion for TestSource
    struct TestPackageVersion : public IPackageVersion
    {
        TestPackageVersion(const Manifest& manifest) : m_manifest(manifest) {}
        LocIndString GetProperty(PackageVersionProperty property) const override;
        std::vector<LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override;
        Manifest GetManifest() const override;
        std::map<std::string, std::string> GetInstallationMetadata() const override;

        Manifest m_manifest;
    };

    // IPackage for TestSource
    struct TestPackage : public IPackage
    {
        TestPackage(const Manifest& manifest) : m_manifest(manifest) {}

        LocIndString GetProperty(PackageProperty property) const override
        {
            switch (property)
            {
            case PackageProperty::Id:
                return LocIndString{ m_manifest.Id };
            case PackageProperty::Name:
                return LocIndString{ m_manifest.Name };
            default:
                return {};
            }
        }

        std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
        {
            return {};
        }

        std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
        {
            return { { "", m_manifest.Version, m_manifest.Channel } };
        }

        std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
        {
            return std::make_shared<TestPackageVersion>(m_manifest);
        }

        std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
        {
            if ((versionKey.Version.empty() || versionKey.Version == m_manifest.Version) &&
                (versionKey.Channel.empty() || versionKey.Channel == m_manifest.Channel))
            {
                return std::make_shared<TestPackageVersion>(m_manifest);
            }
            else
            {
                return {};
            }
        }

        bool IsUpdateAvailable() const override
        {
            return false;
        }

        Manifest m_manifest;
    };

    // An ISource implementation for use across the test code.
    struct TestSource : public ISource
    {
        const SourceDetails& GetDetails() const override;
        const std::string& GetIdentifier() const override;
        SearchResult Search(const SearchRequest& request) const override;
        bool IsComposite() const override;

        SourceDetails Details;
        std::string Identifier = "*TestSource";
        std::function<SearchResult(const SearchRequest& request)> SearchFunction;
        bool Composite = false;
    };
}
