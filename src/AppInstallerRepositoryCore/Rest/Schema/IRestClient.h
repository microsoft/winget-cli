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
        std::string packageIdentifier;
        std::string packageName;
        std::string publisher;

        PackageInfo(std::string packageIdentifier, std::string packageName, std::string publisher) 
        : packageIdentifier(packageIdentifier), packageName(packageName), publisher(publisher) {}
    };

    // Minimal information retrieved for any search request.
    struct Package
    {
        PackageInfo packageInfo;
        std::vector<AppInstaller::Utility::VersionAndChannel> versions;

        Package(PackageInfo packageInfo, std::vector<AppInstaller::Utility::VersionAndChannel> versions)
        : packageInfo(packageInfo), versions(versions) {}
    };

    struct SearchResult
    {
        std::vector<Package> Matches;
        bool Truncated = false;
    };

    // Performs a search based on the given criteria.
    virtual SearchResult Search(const SearchRequest& request) const = 0;

    // Gets the manifest for given version
    virtual std::optional<Manifest::Manifest> GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const = 0;
    };
}
