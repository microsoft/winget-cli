// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteVersion.h>
#include <winget/Manifest.h>
#include <winget/RepositorySearch.h>
#include <AppInstallerVersions.h>
#include <winget/Authentication.h>
#include <vector>

namespace AppInstaller::Repository::Rest::Schema
{
    // The common interface used to interact with RestAPI responses.
    struct IRestClient
    {
    virtual ~IRestClient() = default;

    struct PackageInfo
    {
        std::string PackageIdentifier;
        std::string PackageName;
        std::string Publisher;

        PackageInfo(std::string packageIdentifier, std::string packageName, std::string publisher) 
        : PackageIdentifier(std::move(packageIdentifier)), PackageName(std::move(packageName)), Publisher(std::move(publisher)) {}
    };

    // NOTE: When changes are made to VersionInfo struct, remember to update the OptimizedSearch path in RestInterface1_0
    // where VersionInfo struct was directly created from manifest.
    struct VersionInfo
    {
        AppInstaller::Utility::VersionAndChannel VersionAndChannel;
        std::optional<Manifest::Manifest> Manifest;
        std::vector<std::string> PackageFamilyNames;
        std::vector<std::string> ProductCodes;
        std::vector<AppInstaller::Utility::Version> ArpVersions;
        std::vector<std::string> UpgradeCodes;

        VersionInfo(AppInstaller::Utility::VersionAndChannel versionAndChannel, std::optional<Manifest::Manifest> manifest, std::vector<std::string> packageFamilyNames = {}, std::vector<std::string> productCodes = {}, std::vector<AppInstaller::Utility::Version> arpVersions = {}, std::vector<std::string> upgradeCodes = {})
            : VersionAndChannel(std::move(versionAndChannel)), Manifest(std::move(manifest)), PackageFamilyNames(std::move(packageFamilyNames)), ProductCodes(std::move(productCodes)), ArpVersions(std::move(arpVersions)), UpgradeCodes(std::move(upgradeCodes)) {}
    };

    // Minimal information retrieved for any search request.
    struct Package
    {
        PackageInfo PackageInformation;
        std::vector<VersionInfo> Versions;

        Package(PackageInfo packageInfo, std::vector<VersionInfo> versions)
        : PackageInformation(std::move(packageInfo)), Versions(std::move(versions)) {}
    };

    struct SearchResult
    {
        std::vector<Package> Matches;
        bool Truncated = false;
    };

    struct SourceAgreementEntry
    {
        std::string Label;
        std::string Text;
        std::string Url;
    };

    // Information endpoint models
    struct Information
    {
        std::string SourceIdentifier;
        std::vector<std::string> ServerSupportedVersions;
        std::string SourceAgreementsIdentifier;
        std::vector<SourceAgreementEntry> SourceAgreements;
        std::vector<std::string> UnsupportedPackageMatchFields;
        std::vector<std::string> RequiredPackageMatchFields;
        std::vector<std::string> UnsupportedQueryParameters;
        std::vector<std::string> RequiredQueryParameters;
        Authentication::AuthenticationInfo Authentication;

        Information() {}
        Information(std::string sourceId, std::vector<std::string> versions)
            : SourceIdentifier(std::move(sourceId)), ServerSupportedVersions(std::move(versions)) {}
    };

    // Get interface version.
    virtual Utility::Version GetVersion() const = 0;

    // Get source information.
    virtual Information GetSourceInformation() const = 0;

    // Performs a search based on the given criteria.
    virtual SearchResult Search(const SearchRequest& request) const = 0;

    // Gets the manifest for given version
    virtual std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const = 0;
    
    // Gets the manifests for given query parameters
    virtual std::vector<Manifest::Manifest> GetManifests(const std::string& packageId, const std::map<std::string_view, std::string>& params = {}) const = 0;
    };
}
