// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include "PackageCollection.h"

#include "AppInstallerRuntime.h"
#include "winget/JsonSchemaValidation.h"

#include "PackagesSchema.h"

#include <algorithm>
#include <ostream>

using namespace AppInstaller::Repository;

namespace AppInstaller::CLI
{
    namespace
    {
        // Strings used in the Packages JSON file.
        // Most will be used to access a JSON value, so they need to be std::string
        struct StaticStrings
        {
            const std::string PackagesJson_Schema = "$schema";
            const std::string PackagesJson_SchemaUri_v1_0 = "https://aka.ms/winget-packages.schema.1.0.json";
            const std::string PackagesJson_SchemaUri_v2_0 = "https://aka.ms/winget-packages.schema.2.0.json";
            const std::string PackagesJson_WinGetVersion = "WinGetVersion";
            const std::string PackagesJson_CreationDate = "CreationDate";

            const std::string PackagesJson_Sources = "Sources";
            const std::string PackagesJson_Source_Details = "SourceDetails";
            const std::string PackagesJson_Source_Name = "Name";
            const std::string PackagesJson_Source_Identifier = "Identifier";
            const std::string PackagesJson_Source_Argument = "Argument";
            const std::string PackagesJson_Source_Type = "Type";

            const std::string PackagesJson_Packages = "Packages";
            const std::string PackagesJson_Package_Id = "Id";
            const std::string PackagesJson_Package_PackageIdentifier = "PackageIdentifier";
            const std::string PackagesJson_Package_Version = "Version";
            const std::string PackagesJson_Package_Channel = "Channel";
            const std::string PackagesJson_Package_Scope = "Scope";

            static const StaticStrings& Instance()
            {
                static StaticStrings instance;
                return instance;
            }
        };

        // Gets or creates a property of a JSON object by its name.
        Json::Value& GetJsonProperty(Json::Value& node, const std::string& propertyName, Json::ValueType valueType)
        {
            if (!node.isMember(propertyName))
            {
                node[propertyName] = Json::Value{ valueType };
            }
            else
            {
                THROW_HR_IF(E_NOT_VALID_STATE, node[propertyName].type() != valueType);
            }

            return node[propertyName];
        }

        // The interface for a package collection parser.
        struct IPackageCollectionParser
        {
            virtual ~IPackageCollectionParser() = default;

            virtual PackageCollection Parse(const Json::Value& root) = 0;
        };

        // The parsing code for schema v1.0
        struct PackageCollectionParser_1_0 : public IPackageCollectionParser
        {
            PackageCollection Parse(const Json::Value& root) override
            {
                PackageCollection result;

                // Regardless of the fact that the value is required in 1.0, allow it to be optional
                if (root.isMember(ss.PackagesJson_WinGetVersion))
                {
                    result.ClientVersion = root[ss.PackagesJson_WinGetVersion].asString();
                }

                for (const auto& sourceNode : root[ss.PackagesJson_Sources])
                {
                    auto newSource = ParseSourceNode(sourceNode);
                    auto existingSource = std::find_if(result.Sources.begin(), result.Sources.end(), [&](const PackageCollection::Source& s) { return s.Details.Identifier == newSource.Details.Identifier; });
                    if (existingSource == result.Sources.end())
                    {
                        result.Sources.push_back(std::move(newSource));
                    }
                    else
                    {
                        existingSource->Packages.insert(existingSource->Packages.end(), newSource.Packages.begin(), newSource.Packages.end());
                    }
                }

                return result;
            }

        protected:
            // Reads the description of a package from a Package node in the JSON.
            virtual PackageCollection::Package ParsePackageNode(const Json::Value& packageNode)
            {
                std::string id = packageNode[ss.PackagesJson_Package_Id].asString();
                std::string version = packageNode.isMember(ss.PackagesJson_Package_Version) ? packageNode[ss.PackagesJson_Package_Version].asString() : "";
                std::string channel = packageNode.isMember(ss.PackagesJson_Package_Channel) ? packageNode[ss.PackagesJson_Package_Channel].asString() : "";
                std::string scope = packageNode.isMember(ss.PackagesJson_Package_Scope) ? packageNode[ss.PackagesJson_Package_Scope].asString() : "";

                PackageCollection::Package package{ Utility::LocIndString{ id }, Utility::Version{ version }, Utility::Channel{ channel } };
                package.Scope = Manifest::ConvertToScopeEnum(scope);

                return package;
            }

            // Reads the description of a Source and all the packages needed from it, from a Source node in the JSON.
            PackageCollection::Source ParseSourceNode(const Json::Value& sourceNode)
            {
                SourceDetails sourceDetails;
                auto& detailsNode = sourceNode[ss.PackagesJson_Source_Details];
                sourceDetails.Identifier = Utility::LocIndString{ detailsNode[ss.PackagesJson_Source_Identifier].asString() };
                sourceDetails.Name = detailsNode[ss.PackagesJson_Source_Name].asString();
                sourceDetails.Arg = detailsNode[ss.PackagesJson_Source_Argument].asString();
                sourceDetails.Type = detailsNode[ss.PackagesJson_Source_Type].asString();

                PackageCollection::Source source{ std::move(sourceDetails) };
                for (const auto& packageNode : sourceNode[ss.PackagesJson_Packages])
                {
                    source.Packages.emplace_back(ParsePackageNode(packageNode));
                }

                return source;
            }

            const StaticStrings& ss = StaticStrings::Instance();
        };

        // The parsing code for schema v2.0
        struct PackageCollectionParser_2_0 : public PackageCollectionParser_1_0
        {
        protected:
            // Reads the description of a package from a Package node in the JSON.
            PackageCollection::Package ParsePackageNode(const Json::Value& packageNode) override
            {
                std::string id = packageNode[ss.PackagesJson_Package_PackageIdentifier].asString();
                std::string version = packageNode.isMember(ss.PackagesJson_Package_Version) ? packageNode[ss.PackagesJson_Package_Version].asString() : "";
                std::string channel = packageNode.isMember(ss.PackagesJson_Package_Channel) ? packageNode[ss.PackagesJson_Package_Channel].asString() : "";
                std::string scope = packageNode.isMember(ss.PackagesJson_Package_Scope) ? packageNode[ss.PackagesJson_Package_Scope].asString() : "";

                PackageCollection::Package package{ Utility::LocIndString{ id }, Utility::Version{ version }, Utility::Channel{ channel } };
                package.Scope = Manifest::ConvertToScopeEnum(scope);

                return package;
            }
        };

        // Creates a minimal root object of a Packages JSON file.
        Json::Value CreateRoot(const std::string& wingetVersion)
        {
            const auto& ss = StaticStrings::Instance();

            Json::Value root{ Json::ValueType::objectValue };
            root[ss.PackagesJson_WinGetVersion] = wingetVersion;
            // We only generate the latest schema
            root[ss.PackagesJson_Schema] = ss.PackagesJson_SchemaUri_v2_0;

            std::stringstream currentTimeStream;
            Utility::OutputTimePoint(currentTimeStream, std::chrono::system_clock::now(), true);
            root[ss.PackagesJson_CreationDate] = currentTimeStream.str();

            return root;
        }

        // Adds a new Package node to a Source node in the Json file, and returns it.
        Json::Value& AddPackageToSource(Json::Value& sourceNode, const PackageCollection::Package& package)
        {
            const auto& ss = StaticStrings::Instance();

            Json::Value packageNode{ Json::ValueType::objectValue };
            packageNode[ss.PackagesJson_Package_PackageIdentifier] = package.Id.get();

            // Only add version and channel if present.
            // Packages may not have a channel, or versions may not have been requested.
            const std::string& version = package.VersionAndChannel.GetVersion().ToString();
            if (!version.empty())
            {
                packageNode[ss.PackagesJson_Package_Version] = version;
            }

            const std::string& channel = package.VersionAndChannel.GetChannel().ToString();
            if (!channel.empty())
            {
                packageNode[ss.PackagesJson_Package_Channel] = channel;
            }

            if (package.Scope != Manifest::ScopeEnum::Unknown)
            {
                packageNode[ss.PackagesJson_Package_Scope] = std::string{ Manifest::ScopeToString(package.Scope) };
            }

            return sourceNode[ss.PackagesJson_Packages].append(std::move(packageNode));
        }

        // Adds a new Source node to the JSON, and returns it.
        Json::Value& AddSourceNode(Json::Value& root, const PackageCollection::Source& source)
        {
            const auto& ss = StaticStrings::Instance();

            Json::Value sourceNode{ Json::ValueType::objectValue };

            Json::Value sourceDetailsNode{ Json::ValueType::objectValue };
            sourceDetailsNode[ss.PackagesJson_Source_Name] = source.Details.Name;
            sourceDetailsNode[ss.PackagesJson_Source_Argument] = source.Details.Arg;
            sourceDetailsNode[ss.PackagesJson_Source_Identifier] = source.Details.Identifier;
            sourceDetailsNode[ss.PackagesJson_Source_Type] = source.Details.Type;
            sourceNode[ss.PackagesJson_Source_Details] = std::move(sourceDetailsNode);

            sourceNode[ss.PackagesJson_Packages] = Json::Value{ Json::ValueType::arrayValue };

            auto& sourcesNode = GetJsonProperty(root, ss.PackagesJson_Sources, Json::ValueType::arrayValue);
            for (const auto& package : source.Packages)
            {
                AddPackageToSource(sourceNode, package);
            }

            return sourcesNode.append(std::move(sourceNode));
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

        ParseResult TryParseJson(const Json::Value& root)
        {
            const auto& ss = StaticStrings::Instance();

            // Find the schema used for the JSON
            if (!(root.isObject() && root.isMember(ss.PackagesJson_Schema) && root[ss.PackagesJson_Schema].isString()))
            {
                AICLI_LOG(CLI, Error, << "Import file is missing \"" << ss.PackagesJson_Schema << "\" property");
                return ParseResult{ ParseResult::Type::MissingSchema };
            }

            const auto& schemaUri = root[ss.PackagesJson_Schema].asString();
            Json::Value schemaJson;
            std::unique_ptr<IPackageCollectionParser> parser;
            if (schemaUri == ss.PackagesJson_SchemaUri_v1_0)
            {
                schemaJson = JsonSchema::LoadResourceAsSchemaDoc(MAKEINTRESOURCE(IDX_PACKAGES_SCHEMA_V1), MAKEINTRESOURCE(PACKAGESSCHEMA_RESOURCE_TYPE));
                parser = std::make_unique<PackageCollectionParser_1_0>();
            }
            else if (schemaUri == ss.PackagesJson_SchemaUri_v2_0)
            {
                schemaJson = JsonSchema::LoadResourceAsSchemaDoc(MAKEINTRESOURCE(IDX_PACKAGES_SCHEMA_V2), MAKEINTRESOURCE(PACKAGESSCHEMA_RESOURCE_TYPE));
                parser = std::make_unique<PackageCollectionParser_2_0>();
            }
            else
            {
                AICLI_LOG(CLI, Error, << "Unrecognized schema for import file: " << schemaUri);
                return ParseResult{ ParseResult::Type::UnrecognizedSchema };
            }

            // Validate the JSON against the schema.
            valijson::Schema schema;
            JsonSchema::PopulateSchema(schemaJson, schema);

            valijson::ValidationResults results;
            if (!JsonSchema::Validate(schema, root, results))
            {
                return ParseResult{ ParseResult::Type::SchemaValidationFailed, JsonSchema::GetErrorStringFromResults(results) };
            }

            // Extract the data from the JSON.
            return ParseResult{ parser->Parse(root) };
        }
    }
}