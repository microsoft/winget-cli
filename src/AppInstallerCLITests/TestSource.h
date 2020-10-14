// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Public/AppInstallerRepositorySource.h>
#include <winget/Manifest.h>

#include <functional>

namespace TestCommon
{
    // IPackageVersion for TestSource
    struct TestPackageVersion : public AppInstaller::Repository::IPackageVersion
    {
        TestPackageVersion(const AppInstaller::Manifest::Manifest& manifest);

        AppInstaller::Utility::LocIndString GetProperty(AppInstaller::Repository::PackageVersionProperty property) const override;
        std::vector<AppInstaller::Utility::LocIndString> GetMultiProperty(AppInstaller::Repository::PackageVersionMultiProperty property) const override;
        AppInstaller::Manifest::Manifest GetManifest() const override;
        std::map<std::string, std::string> GetInstallationMetadata() const override;

        AppInstaller::Manifest::Manifest m_manifest;
    };

    // IPackage for TestSource
    struct TestPackage : public AppInstaller::Repository::IPackage
    {
        TestPackage(const AppInstaller::Manifest::Manifest& manifest);

        AppInstaller::Utility::LocIndString GetProperty(AppInstaller::Repository::PackageProperty property) const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetInstalledVersion() const override;
        std::vector<AppInstaller::Repository::PackageVersionKey> GetAvailableVersionKeys() const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetLatestAvailableVersion() const override;
        std::shared_ptr<AppInstaller::Repository::IPackageVersion> GetAvailableVersion(const AppInstaller::Repository::PackageVersionKey& versionKey) const override;
        bool IsUpdateAvailable() const override;

        AppInstaller::Manifest::Manifest m_manifest;
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
