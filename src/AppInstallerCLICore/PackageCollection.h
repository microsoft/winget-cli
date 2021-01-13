// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "AppInstallerDateTime.h"

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
        PackageRequestsFromSource(const Utility::LocIndString& sourceIdentifier, const SourceDetails& sourceDetails);
        PackageRequestsFromSource(Utility::LocIndString&& sourceIdentifier, SourceDetails&& sourceDetails);

        Utility::LocIndString SourceIdentifier;
        SourceDetails Details;
        std::vector<PackageRequest> Packages;
    };

    // Container for data to identify multiple packages to be installed from multiple sources.
    struct PackageCollection
    {
        // Version of the WinGet client that produced this request.
        std::string ClientVersion;

        // Requests from each individual source.
        std::vector<PackageRequestsFromSource> RequestsFromSources;
    };

    namespace PackagesJson
    {
        // Strings used in the Packages JSON file.
        // Most will be used to access a JSON value, so they need to be std::string
        const std::string SCHEMA_PROPERTY = "$schema";
        const std::string SCHEMA_PATH = "https://aka.ms/winget-packages.schema.json";
        const std::string WINGET_VERSION_PROPERTY = "WinGetVersion";
        const std::string CREATION_DATE_PROPERTY = "CreationDate";

        const std::string SOURCES_PROPERTY = "Sources";
        const std::string SOURCE_DETAILS_PROPERTY = "SourceDetails";
        const std::string SOURCE_NAME_PROPERTY = "Name";
        const std::string SOURCE_IDENTIFIER_PROPERTY = "Identifier";
        const std::string SOURCE_ARGUMENT_PROPERTY = "Argument";
        const std::string SOURCE_TYPE_PROPERTY = "Type";

        const std::string PACKAGES_PROPERTY = "Packages";
        const std::string PACKAGE_ID_PROPERTY = "Id";
        const std::string PACKAGE_VERSION_PROPERTY = "Version";
        const std::string PACKAGE_CHANNEL_PROPERTY = "Channel";

        // Creates a minimal root object of a Packages JSON file.
        Json::Value CreateRoot();

        // Adds a new Source node to the JSON, and returns it.
        Json::Value& AddSourceNode(Json::Value& root, const PackageRequestsFromSource& source);

        // Adds a new Package node to a Source node in the Json file, and returns it.
        Json::Value& AddPackageToSource(Json::Value& source, const PackageRequest& package);

        // Converts a collection of packages to its JSON representation for exporting.
        Json::Value CreateJson(const PackageCollection& packages);

        PackageCollection ParseJson(const Json::Value& root);
    }
}