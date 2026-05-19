// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ManifestDeserializer.h"
#include <winget/JsonUtil.h>

using namespace AppInstaller::Manifest;

namespace AppInstaller::Repository::Rest::Schema::V1_28::Json
{
    namespace
    {
        constexpr std::string_view DesiredStateConfiguration = "DesiredStateConfiguration"sv;
        constexpr std::string_view PowerShell = "PowerShell"sv;
        constexpr std::string_view DSCv3 = "DSCv3"sv;
        constexpr std::string_view RepositoryUrl = "RepositoryUrl"sv;
        constexpr std::string_view ModuleName = "ModuleName"sv;
        constexpr std::string_view Resources = "Resources"sv;
        constexpr std::string_view Name = "Name"sv;
        constexpr std::string_view Type = "Type"sv;
    }

    std::optional<Manifest::ManifestInstaller> ManifestDeserializer::DeserializeInstaller(const web::json::value& installerJsonObject) const
    {
        auto result = V1_12::Json::ManifestDeserializer::DeserializeInstaller(installerJsonObject);

        if (result)
        {
            auto& installer = result.value();

            // DesiredStateConfiguration
            auto dscNode = JSON::GetJsonValueFromNode(installerJsonObject, JSON::GetUtilityString(DesiredStateConfiguration));
            if (dscNode)
            {
                const auto& dscObject = dscNode->get();
                if (!dscObject.is_null() && dscObject.is_object())
                {
                    // PowerShell DSC modules
                    auto powerShellNode = JSON::GetRawJsonArrayFromJsonNode(dscObject, JSON::GetUtilityString(PowerShell));
                    if (powerShellNode)
                    {
                        for (auto const& moduleNode : powerShellNode->get())
                        {
                            std::optional<std::string> repositoryUrl = JSON::GetRawStringValueFromJsonNode(moduleNode, JSON::GetUtilityString(RepositoryUrl));
                            std::optional<std::string> moduleName = JSON::GetRawStringValueFromJsonNode(moduleNode, JSON::GetUtilityString(ModuleName));

                            if (!JSON::IsValidNonEmptyStringValue(repositoryUrl) || !JSON::IsValidNonEmptyStringValue(moduleName))
                            {
                                AICLI_LOG(Repo, Error, << "Missing required fields in DesiredStateConfiguration PowerShell entry.");
                                continue;
                            }

                            std::vector<DesiredStateConfigurationResourceInfo> resources;
                            auto resourcesNode = JSON::GetRawJsonArrayFromJsonNode(moduleNode, JSON::GetUtilityString(Resources));
                            if (resourcesNode)
                            {
                                for (auto const& resourceNode : resourcesNode->get())
                                {
                                    std::optional<std::string> name = JSON::GetRawStringValueFromJsonNode(resourceNode, JSON::GetUtilityString(Name));
                                    if (JSON::IsValidNonEmptyStringValue(name))
                                    {
                                        DesiredStateConfigurationResourceInfo resourceInfo;
                                        resourceInfo.Name = std::move(*name);
                                        resources.emplace_back(std::move(resourceInfo));
                                    }
                                }
                            }

                            if (!resources.empty())
                            {
                                installer.DesiredStateConfiguration.emplace_back(std::move(*repositoryUrl), std::move(*moduleName), std::move(resources));
                            }
                        }
                    }

                    // DSCv3 resources
                    auto dscv3Node = JSON::GetJsonValueFromNode(dscObject, JSON::GetUtilityString(DSCv3));
                    if (dscv3Node)
                    {
                        const auto& dscv3Object = dscv3Node->get();
                        if (!dscv3Object.is_null() && dscv3Object.is_object())
                        {
                            std::vector<DesiredStateConfigurationResourceInfo> resources;
                            auto resourcesNode = JSON::GetRawJsonArrayFromJsonNode(dscv3Object, JSON::GetUtilityString(Resources));
                            if (resourcesNode)
                            {
                                for (auto const& resourceNode : resourcesNode->get())
                                {
                                    std::optional<std::string> type = JSON::GetRawStringValueFromJsonNode(resourceNode, JSON::GetUtilityString(Type));
                                    if (JSON::IsValidNonEmptyStringValue(type))
                                    {
                                        DesiredStateConfigurationResourceInfo resourceInfo;
                                        resourceInfo.Name = std::move(*type);
                                        resources.emplace_back(std::move(resourceInfo));
                                    }
                                }
                            }

                            if (!resources.empty())
                            {
                                installer.DesiredStateConfiguration.emplace_back(std::move(resources));
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    Manifest::ManifestVer ManifestDeserializer::GetManifestVersion() const
    {
        return Manifest::s_ManifestVersionV1_28;
    }
}
