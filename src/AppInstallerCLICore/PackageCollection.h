// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "AppInstallerRepositorySearch.h"

#include <json.h>
#include <vector>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Repository;

    // Container for data used to identify a package to be installed.
    struct PackageRequest
    {
        PackageRequest() = default;
        PackageRequest(Utility::LocIndString&& id, Utility::Version&& version, Utility::Channel&& channel);
        PackageRequest(Utility::LocIndString&& id, Utility::VersionAndChannel&& versionAndChannel);

        Utility::LocIndString Id;
        Utility::VersionAndChannel VersionAndChannel;
    };

    // Container for data to identify multiple packages to be installed from a single source.
    struct PackageRequestsFromSource
    {
        PackageRequestsFromSource() = default;
        PackageRequestsFromSource(Utility::LocIndString&& sourceName, Utility::LocIndString&& sourceArg);

        Utility::LocIndString SourceName;
        Utility::LocIndString SourceArg;
        std::vector<PackageRequest> Packages;
    };

    using PackageCollectionRequest = std::vector<PackageRequestsFromSource>;

    // Parses a package collection from a JSON file.
    PackageCollectionRequest ParsePackageCollection(const Json::Value& root);

    // Converts the result of a search to a collection of package
    // requests adequate for exporting.
    PackageCollectionRequest ConvertSearchResultToPackageRequests(const SearchResult& packages);

    // Creates a JSON representing a package collection.
    Json::Value ConvertPackageRequestsToJson(const PackageCollectionRequest& packages);

    namespace PackagesJson
    {
        // Strings used in the Packages JSON file.
        // Most will be used to access a JSON value, so they need to be std::string
        const std::string SCHEMA_PROPERTY = "$schema";
        const std::string SCHEMA_PATH = "https://aka.ms/winget-packages.schema.json";
        const std::string WINGET_VERSION_PROPERTY = "wingetVersion";
        const std::string CREATION_DATE_PROPERTY = "creationDate";

        const std::string SOURCES_PROPERTY = "sources";
        const std::string SOURCE_NAME_PROPERTY = "name";
        const std::string SOURCE_ARGUMENT_PROPERTY = "argument";

        const std::string PACKAGES_PROPERTY = "packages";
        const std::string PACKAGE_ID_PROPERTY = "id";
        const std::string PACKAGE_VERSION_PROPERTY = "version";
        const std::string PACKAGE_CHANNEL_PROPERTY = "channel";

        // Creates a minimal root object of a Packages JSON file.
        Json::Value CreateRoot();

        // Adds a new Source node to the JSON, and returns it.
        Json::Value& AddSourceNode(Json::Value& root, const PackageRequestsFromSource& source);

        // Adds a new Package node to a Source node in the Json file, and returns it.
        Json::Value& AddPackageToSource(Json::Value& source, const PackageRequest& package);
    }
}