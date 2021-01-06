// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageCollection.h"
#include "AppInstallerRepositorySource.h"
#include "AppInstallerRuntime.h"

#include <json.h>
#include <algorithm>
#include <ostream>
#include <chrono>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::PackagesJson;
    using namespace AppInstaller::Repository;

    namespace
    {
        // Gets a property of a JSON object by its name.
        template<Json::ValueType T>
        Json::Value& GetJsonProperty(Json::Value& node, const std::string& propertyName)
        {
            if (!node.isMember(propertyName))
            {
                node[propertyName] = Json::Value{ T };
            }

            THROW_HR_IF(E_NOT_VALID_STATE, node[propertyName].type() != T);
            return node[propertyName];
        }

        // Reads the description of a package from a Package node in the JSON.
        PackageRequest ParsePackageNode(const Json::Value& packageNode)
        {
            std::string id = packageNode[PACKAGE_ID_PROPERTY].asString();
            std::string version = packageNode[PACKAGE_VERSION_PROPERTY].asString();
            std::string channel = packageNode.isMember(PACKAGE_CHANNEL_PROPERTY) ? packageNode[PACKAGE_CHANNEL_PROPERTY].asString() : "";

            PackageRequest packageRequest{ Utility::LocIndString{ id }, Utility::Version{ version }, Utility::Channel{ channel } };

            return packageRequest;
        }

        // Reads the description of a Source and all the packages needed from it, from a Source node in the JSON.
        PackageRequestsFromSource ParseSourceNode(const Json::Value& sourceNode)
        {
            PackageRequestsFromSource requestsFromSource
            {
                Utility::LocIndString{ sourceNode[SOURCE_NAME_PROPERTY].asString() },
                Utility::LocIndString{ sourceNode[SOURCE_ARGUMENT_PROPERTY].asString() }
            };

            for (const auto& packageNode : sourceNode[PACKAGES_PROPERTY])
            {
                requestsFromSource.Packages.push_back(ParsePackageNode(packageNode));
            }

            return requestsFromSource;
        }

        // Gets the available PackageVersion that has the same version as the installed version.
        // The package must have an installed version.
        // Returns null if not available.
        std::shared_ptr<IPackageVersion> GetAvailableVersionMatchingInstalled(const IPackage& package)
        {
            auto installedVersion = package.GetInstalledVersion();
            PackageVersionKey installedVersionKey
            {
                "",
                installedVersion->GetProperty(PackageVersionProperty::Version).get(),
                installedVersion->GetProperty(PackageVersionProperty::Channel).get(),
            };
            return package.GetAvailableVersion(installedVersionKey);
        }
    }

    namespace PackagesJson
    {
        Json::Value CreateRoot()
        {
            Json::Value root{ Json::ValueType::objectValue };
            root[WINGET_VERSION_PROPERTY] = Runtime::GetClientVersion().get();
            root[SCHEMA_PROPERTY] = SCHEMA_PATH;

            // TODO: Clean up. Do we need this?
            std::time_t currentTimeTT = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm currentTimeTM;
            std::stringstream currentTimeStream;
            gmtime_s(&currentTimeTM, &currentTimeTT);
            currentTimeStream << std::put_time(&currentTimeTM, "%c");
            root[CREATION_DATE_PROPERTY] = currentTimeStream.str();

            return root;
        }

        Json::Value& AddSourceNode(Json::Value& root, const PackageRequestsFromSource& source)
        {
            Json::Value sourceNode{ Json::ValueType::objectValue };
            sourceNode[SOURCE_NAME_PROPERTY] = source.SourceName.get();
            sourceNode[SOURCE_ARGUMENT_PROPERTY] = source.SourceArg.get();
            sourceNode[PACKAGES_PROPERTY] = Json::Value{ Json::ValueType::arrayValue };

            auto& sourcesNode = GetJsonProperty<Json::ValueType::arrayValue>(root, SOURCES_PROPERTY);
            for (const auto& package : source.Packages)
            {
                AddPackageToSource(sourceNode, package);
            }

            return sourcesNode.append(std::move(sourceNode));
        }

        Json::Value& AddPackageToSource(Json::Value& sourceNode, const PackageRequest& package)
        {
            Json::Value packageNode{ Json::ValueType::objectValue };
            packageNode[PACKAGE_ID_PROPERTY] = package.Id.get();
            packageNode[PACKAGE_VERSION_PROPERTY] = package.VersionAndChannel.GetVersion().ToString();

            // Only add channel if present
            const std::string& channel = package.VersionAndChannel.GetChannel().ToString();
            if (!channel.empty())
            {
                packageNode[PACKAGE_CHANNEL_PROPERTY] = channel;
            }

            return sourceNode[PACKAGES_PROPERTY].append(std::move(packageNode));
        }
    }

    PackageRequest::PackageRequest(Utility::LocIndString&& id, Utility::Version&& version, Utility::Channel&& channel) :
        Id(std::move(id)), VersionAndChannel(std::move(version), std::move(channel)) {}

    PackageRequest::PackageRequest(Utility::LocIndString&& id, Utility::VersionAndChannel&& versionAndChannel) :
        Id(std::move(id)), VersionAndChannel(std::move(versionAndChannel)) {}

    PackageRequestsFromSource::PackageRequestsFromSource(Utility::LocIndString&& sourceName, Utility::LocIndString&& sourceArg) :
        SourceName(std::move(sourceName)), SourceArg(std::move(sourceArg)) {}

    std::vector<PackageRequestsFromSource> ParsePackageCollection(const Json::Value& root)
    {
        // TODO: Validate schema. The following assumes the file is already valid.
        // TODO: Use version & creation date?

        std::vector<PackageRequestsFromSource> requests = {};
        for (const auto& sourceNode : root[SOURCES_PROPERTY])
        {
            requests.push_back(ParseSourceNode(sourceNode));
        }

        return requests;
    }

    std::vector<PackageRequestsFromSource> ConvertSearchResultToPackageRequests(const SearchResult& packages)
    {
        std::vector<PackageRequestsFromSource> requests = {};
        for (const auto& packageMatch : packages.Matches)
        {
            auto availableVersion = GetAvailableVersionMatchingInstalled(*packageMatch.Package);
            if (!availableVersion)
            {
                AICLI_LOG(CLI, Info, << "No available package found for " << packageMatch.Package->GetProperty(PackageProperty::Id));
                continue;
            }

            auto sourceDetails = availableVersion->GetSource()->GetDetails();
            auto sourceItr = std::find_if(requests.begin(), requests.end(), [&](const PackageRequestsFromSource& s) { return s.SourceName == sourceDetails.Name; });
            if (sourceItr == requests.end())
            {
                requests.emplace_back(Utility::LocIndString{ sourceDetails.Name }, Utility::LocIndString{ sourceDetails.Arg });
                sourceItr = std::prev(requests.end());
            }

            sourceItr->Packages.emplace_back(
                availableVersion->GetProperty(PackageVersionProperty::Id),
                availableVersion->GetProperty(PackageVersionProperty::Version).get(),
                availableVersion->GetProperty(PackageVersionProperty::Channel).get());
        }

        return requests;
    }

    Json::Value ConvertPackageRequestsToJson(const PackageCollectionRequest& packages)
    {
        Json::Value root = CreateRoot();
        for (const auto& source : packages)
        {
            AddSourceNode(root, source);
        }

        return root;
    }
}