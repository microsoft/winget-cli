// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/Schema/Version.h"
#include <AppInstallerVersions.h>
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
        : PackageIdentifier(packageIdentifier), PackageName(packageName), Publisher(publisher) {}
    };

    struct VersionInfo
    {
        AppInstaller::Utility::VersionAndChannel VersionAndChannel;
        Manifest::Manifest Manifest;

        VersionInfo(AppInstaller::Utility::VersionAndChannel versionAndChannel, Manifest::Manifest manifest = {})
            : VersionAndChannel(versionAndChannel), Manifest(manifest) {}
    };

    // Minimal information retrieved for any search request.
    struct Package
    {
        PackageInfo PackageInformation;
        std::vector<VersionInfo> Versions;

        Package(PackageInfo packageInfo, std::vector<VersionInfo> versions)
        : PackageInformation(packageInfo), Versions(versions) {}
    };

    struct SearchResult
    {
        std::vector<Package> Matches;
        bool Truncated = false;
    };

    // Information endpoint models
    struct SupportedVersion
    {
        std::string ApiVersion;
        std::vector<std::string> ObjectVersions;

        SupportedVersion(std::string apiVersion, std::vector<std::string> versions)
            : ApiVersion(apiVersion), ObjectVersions(versions) {}
    };

    struct Information
    {
        std::string SourceIdentifier;
        std::vector<SupportedVersion> ServerSupportedVersions;

        Information(std::string sourceId, std::vector<SupportedVersion> versions)
            : SourceIdentifier(sourceId), ServerSupportedVersions(versions) {}
    };

    // Performs a search based on the given criteria.
    virtual SearchResult Search(const SearchRequest& request) const = 0;

    // Gets the manifest for given version
    virtual std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const = 0;
    };
}
