// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include "PackageCollection.h"
#include "AppInstallerRuntime.h"

#pragma warning (push, 0)
#include "valijson/adapters/jsoncpp_adapter.hpp"
#include "valijson/utils/jsoncpp_utils.hpp"
#include "valijson/schema.hpp"
#include "valijson/schema_parser.hpp"
#include "valijson/validator.hpp"
#pragma warning (pop)

#include <algorithm>
#include <ostream>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::Repository;

    namespace
    {
        // Strings used in the Packages JSON file.
        // Most will be used to access a JSON value, so they need to be std::string
        const std::string s_PackagesJson_Schema = "$schema";
        const std::string s_PackagesJson_SchemaUri_V0 = "https://aka.ms/winget-packages-v0.schema.json";
        const std::string s_PackagesJson_WinGetVersion = "WinGetVersion";
        const std::string s_PackagesJson_CreationDate = "CreationDate";

        const std::string s_PackagesJson_Sources = "Sources";
        const std::string s_PackagesJson_Source_Details = "SourceDetails";
        const std::string s_PackagesJson_Source_Name = "Name";
        const std::string s_PackagesJson_Source_Identifier = "Identifier";
        const std::string s_PackagesJson_Source_Argument = "Argument";
        const std::string s_PackagesJson_Source_Type = "Type";

        const std::string s_PackagesJson_Packages = "Packages";
        const std::string s_PackagesJson_Package_Id = "Id";
        const std::string s_PackagesJson_Package_Version = "Version";
        const std::string s_PackagesJson_Package_Channel = "Channel";

        // Gets or creates a property of a JSON object by its name.
        Json::Value& GetJsonProperty(Json::Value& node, const std::string& propertyName, Json::ValueType valueType)
        {
            if (!node.isMember(propertyName))
            {
                node[propertyName] = Json::Value{ valueType };
            }

            THROW_HR_IF(E_NOT_VALID_STATE, node[propertyName].type() != valueType);
            return node[propertyName];
        }

        // Reads the description of a package from a Package node in the JSON.
        PackageCollection::Package ParsePackageNode(const Json::Value& packageNode)
        {
            std::string id = packageNode[s_PackagesJson_Package_Id].asString();
            std::string version = packageNode.isMember(s_PackagesJson_Package_Version) ? packageNode[s_PackagesJson_Package_Version].asString() : "";
            std::string channel = packageNode.isMember(s_PackagesJson_Package_Channel) ? packageNode[s_PackagesJson_Package_Channel].asString() : "";

            PackageCollection::Package package{ Utility::LocIndString{ id }, Utility::Version{ version }, Utility::Channel{ channel } };

            return package;
        }

        // Reads the description of a Source and all the packages needed from it, from a Source node in the JSON.
        PackageCollection::Source ParseSourceNode(const Json::Value& sourceNode)
        {
            SourceDetails sourceDetails;
            auto& detailsNode = sourceNode[s_PackagesJson_Source_Details];
            sourceDetails.Identifier = Utility::LocIndString{ detailsNode[s_PackagesJson_Source_Identifier].asString() };
            sourceDetails.Name = detailsNode[s_PackagesJson_Source_Name].asString();
            sourceDetails.Arg = detailsNode[s_PackagesJson_Source_Argument].asString();
            sourceDetails.Type = detailsNode[s_PackagesJson_Source_Type].asString();

            PackageCollection::Source source{ std::move(sourceDetails) };
            for (const auto& packageNode : sourceNode[s_PackagesJson_Packages])
            {
                source.Packages.push_back(ParsePackageNode(packageNode));
            }

            return source;
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

        // Creates a minimal root object of a Packages JSON file.
        Json::Value CreateRoot(const std::string& wingetVersion)
        {
            Json::Value root{ Json::ValueType::objectValue };
            root[s_PackagesJson_WinGetVersion] = wingetVersion;
            root[s_PackagesJson_Schema] = s_PackagesJson_SchemaUri_V0;

            // TODO: This uses localtime. Do we want to use UTC or add time zone?
            std::stringstream currentTimeStream;
            Utility::OutputTimePoint(currentTimeStream, std::chrono::system_clock::now());
            root[s_PackagesJson_CreationDate] = currentTimeStream.str();

            return root;
        }

        // Adds a new Package node to a Source node in the Json file, and returns it.
        Json::Value& AddPackageToSource(Json::Value& sourceNode, const PackageCollection::Package& package)
        {
            Json::Value packageNode{ Json::ValueType::objectValue };
            packageNode[s_PackagesJson_Package_Id] = package.Id.get();
            packageNode[s_PackagesJson_Package_Version] = package.VersionAndChannel.GetVersion().ToString();

            // Only add channel if present
            const std::string& channel = package.VersionAndChannel.GetChannel().ToString();
            if (!channel.empty())
            {
                packageNode[s_PackagesJson_Package_Channel] = channel;
            }

            return sourceNode[s_PackagesJson_Packages].append(std::move(packageNode));
        }

        // Adds a new Source node to the JSON, and returns it.
        Json::Value& AddSourceNode(Json::Value& root, const PackageCollection::Source& source)
        {
            Json::Value sourceNode{ Json::ValueType::objectValue };


            Json::Value sourceDetailsNode{ Json::ValueType::objectValue };
            sourceDetailsNode[s_PackagesJson_Source_Name] = source.Details.Name;
            sourceDetailsNode[s_PackagesJson_Source_Argument] = source.Details.Arg;
            sourceDetailsNode[s_PackagesJson_Source_Identifier] = source.Details.Identifier;
            sourceDetailsNode[s_PackagesJson_Source_Type] = source.Details.Type;
            sourceNode[s_PackagesJson_Source_Details] = std::move(sourceDetailsNode);

            sourceNode[s_PackagesJson_Packages] = Json::Value{ Json::ValueType::arrayValue };

            auto& sourcesNode = GetJsonProperty(root, s_PackagesJson_Sources, Json::ValueType::arrayValue);
            for (const auto& package : source.Packages)
            {
                AddPackageToSource(sourceNode, package);
            }

            return sourcesNode.append(std::move(sourceNode));
        }

        bool IsValidJson(const Json::Value& document, const std::filesystem::path& schemaPath)
        {
            Json::Value schemaDocument;
            if (!valijson::utils::loadDocument(schemaPath.string(), schemaDocument))
            {
                // TODO: fail
            }

            valijson::Schema schema;
            valijson::SchemaParser parser;
            valijson::adapters::JsonCppAdapter schemaAdapter{ schemaDocument };
            parser.populateSchema(schemaAdapter, schema);

            valijson::Validator validator;
            valijson::ValidationResults results;
             if (validator.validate(schema, valijson::adapters::JsonCppAdapter{ document }, &results)) {
                return true;
            }

            for (const auto& result : results)
            {
                std::stringstream ss;
                ss << result.description << ' ';
                for (const auto& context : result.context)
                {
                    ss << '\\' << context;
                }

                AICLI_LOG(CLI, Error, << "JSON file is invalid: " << ss.str());
            }

            return false;
        }
    }

    namespace PackagesJson
    {
        Json::Value CreateJson(const PackageCollection& packages)
        {
            Json::Value root = CreateRoot(packages.ClientVersion);
            for (const auto& source : packages.Sources)
            {
                AddSourceNode(root, source);
            }

            return root;
        }

        std::optional<PackageCollection> ParseJson(const Json::Value& root)
        {
            // TODO: Embed schema in binaries
            if (!IsValidJson(root, "C:\\src\\winget-cli\\doc\\packages.schema.json"))
            {
                return {};
            }

            PackageCollection packages;
            packages.ClientVersion = root[s_PackagesJson_WinGetVersion].asString();
            for (const auto& sourceNode : root[s_PackagesJson_Sources])
            {
                auto newSource = ParseSourceNode(sourceNode);
                auto existingSource = std::find_if(packages.Sources.begin(), packages.Sources.end(), [&](const PackageCollection::Source& s) { return s.Details.Identifier == newSource.Details.Identifier; });
                if (existingSource == packages.Sources.end())
                {
                    packages.Sources.push_back(std::move(newSource));
                }
                else
                {
                    existingSource->Packages.insert(existingSource->Packages.end(), newSource.Packages.begin(), newSource.Packages.end());
                }
            }

            return packages;
        }
    }
}