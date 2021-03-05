// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "AppInstallerDateTime.h"
#include "AppInstallerLanguageUtilities.h"
#include "AppInstallerRepositorySource.h"

#include <json.h>

#include <vector>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Repository;

    // Container for data to identify multiple packages to be installed from multiple sources.
    struct PackageCollection
    {
        // Description of a package.
        // Does not represent the actual package, just enough to find and install it.
        struct Package
        {
            Package() = default;
            Package(Utility::LocIndString&& id) :
                Id(std::move(id)) {}
            Package(Utility::LocIndString&& id, Utility::Version&& version, Utility::Channel&& channel) :
                Id(std::move(id)), VersionAndChannel(std::move(version), std::move(channel)) {}
            Package(Utility::LocIndString&& id, Utility::VersionAndChannel&& versionAndChannel) :
                Id(std::move(id)), VersionAndChannel(std::move(versionAndChannel)) {}

            Utility::LocIndString Id;
            Utility::VersionAndChannel VersionAndChannel;
        };

        // A source along with a set of packages available from it.
        struct Source
        {
            Source() = default;
            Source(const SourceDetails& sourceDetails) : Details(sourceDetails) {}
            Source(SourceDetails&& sourceDetails) : Details(std::move(sourceDetails)) {}

            SourceDetails Details;
            std::vector<Package> Packages;
        };

        // Version of the WinGet client that produced this collection.
        std::string ClientVersion;

        // Requests from each individual source.
        std::vector<Source> Sources;
    };

    namespace PackagesJson
    {
        struct ParseResult
        {
            enum class Type
            {
                MissingSchema,
                UnrecognizedSchema,
                SchemaValidationFailed,
                Success,
            };

            ParseResult(Type result) : Result(result) {}
            ParseResult(Type result, std::string_view errors) : Result(result), Errors(errors) {}
            ParseResult(PackageCollection&& packages) : Result(Type::Success), Packages(std::move(packages)) {}

            Type Result;
            PackageCollection Packages;
            std::string Errors;
        };

        // Converts a collection of packages to its JSON representation for exporting.
        Json::Value CreateJson(const PackageCollection& packages);

        // Tries to parse a JSON into a collection of packages.
        ParseResult TryParseJson(const Json::Value& root);
    }
}