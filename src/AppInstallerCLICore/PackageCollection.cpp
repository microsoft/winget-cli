// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include "PackageCollection.h"
#include "AppInstallerRuntime.h"

#include <algorithm>
#include <ostream>

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

        // Sets a property of a JSON object to a string if it is not empty
        void SetJsonProperty(Json::Value& node, const std::string& propertyName, const std::string& value)
        {
            if (!value.empty())
            {
                node[propertyName] = value;
            }
            else if (node.isMember(propertyName))
            {
                node.removeMember(propertyName);
            }
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
            PackageRequestsFromSource requestsFromSource;

            if (sourceNode.isMember(SOURCE_DETAILS_PROPERTY))
            {
                auto& detailsNode = sourceNode[SOURCE_DETAILS_PROPERTY];
                requestsFromSource.SourceIdentifier = Utility::LocIndString{ detailsNode[SOURCE_IDENTIFIER_PROPERTY].asString() };
                requestsFromSource.Details.Name = detailsNode[SOURCE_NAME_PROPERTY].asString();
                requestsFromSource.Details.Arg = detailsNode[SOURCE_ARGUMENT_PROPERTY].asString();
                requestsFromSource.Details.Type = detailsNode[SOURCE_TYPE_PROPERTY].asString();
            }

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

            // TODO: This uses localtime. Do we want to use UTC or add time zone?
            std::stringstream currentTimeStream;
            Utility::OutputTimePoint(currentTimeStream, std::chrono::system_clock::now());
            root[CREATION_DATE_PROPERTY] = currentTimeStream.str();

            return root;
        }

        Json::Value& AddSourceNode(Json::Value& root, const PackageRequestsFromSource& source)
        {
            Json::Value sourceNode{ Json::ValueType::objectValue };

            if (!source.Details.Name.empty())
            {
                Json::Value sourceDetailsNode{ Json::ValueType::objectValue };
                sourceDetailsNode[SOURCE_NAME_PROPERTY] = source.Details.Name;
                sourceDetailsNode[SOURCE_ARGUMENT_PROPERTY] = source.Details.Arg;
                sourceDetailsNode[SOURCE_IDENTIFIER_PROPERTY] = source.SourceIdentifier.get();
                sourceDetailsNode[SOURCE_TYPE_PROPERTY] = source.Details.Type;
                sourceNode[SOURCE_DETAILS_PROPERTY] = std::move(sourceDetailsNode);
            }

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

        Json::Value CreateJson(const PackageCollection& packages)
        {
            Json::Value root = PackagesJson::CreateRoot();
            for (const auto& source : packages.RequestsFromSources)
            {
                PackagesJson::AddSourceNode(root, source);
            }

            return root;
        }

        PackageCollection ParseJson(const Json::Value& root)
        {
            // TODO: Validate schema. The following assumes the file is already valid.
            PackageCollection packages;
            packages.ClientVersion = root[WINGET_VERSION_PROPERTY].asString();
            for (const auto& sourceNode : root[SOURCES_PROPERTY])
            {
                // TODO: Prevent duplicates?
                packages.RequestsFromSources.push_back(ParseSourceNode(sourceNode));
            }

            return packages;
        }
    }

    PackageRequest::PackageRequest(Utility::LocIndString&& id, Utility::Version&& version, Utility::Channel&& channel) :
        Id(std::move(id)), VersionAndChannel(std::move(version), std::move(channel)) {}

    PackageRequest::PackageRequest(Utility::LocIndString&& id, Utility::VersionAndChannel&& versionAndChannel) :
        Id(std::move(id)), VersionAndChannel(std::move(versionAndChannel)) {}

    PackageRequestsFromSource::PackageRequestsFromSource(const Utility::LocIndString& sourceIdentifier, const SourceDetails& sourceDetails) :
        SourceIdentifier(sourceIdentifier), Details(sourceDetails) {}

    PackageRequestsFromSource::PackageRequestsFromSource(Utility::LocIndString&& sourceIdentifier, SourceDetails&& sourceDetails) :
        SourceIdentifier(std::move(sourceIdentifier)), Details(std::move(sourceDetails)) {}
}