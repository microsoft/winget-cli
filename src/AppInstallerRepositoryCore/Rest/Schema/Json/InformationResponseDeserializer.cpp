// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include <cpprest/json.h>
#include "JsonHelper.h"
#include "InformationResponseDeserializer.h"
#include "CommonRestConstants.h"

namespace AppInstaller::Repository::Rest::Schema::Json
{
    namespace
    {
        // Information response constants
        constexpr std::string_view SourceIdentifier = "SourceIdentifier"sv;
        constexpr std::string_view ServerSupportedVersions = "ServerSupportedVersions"sv;
        constexpr std::string_view APIVersion = "APIVersion"sv;
        constexpr std::string_view ObjectVersions = "ObjectVersions"sv;
    }

    std::optional<IRestClient::Information> InformationResponseDeserializer::Deserialize(const web::json::value& dataObject) const
    {
        // Get information result from json output.
        std::optional<std::reference_wrapper<const web::json::value>> data = JsonHelper::GetJsonValueFromNode(dataObject, JsonHelper::GetJsonKeyNameString(Data));
        if (!data.has_value())
        {
            AICLI_LOG(Repo, Verbose, << "Missing data");
            return {};
        }

        auto& dataValue = data.value().get();
        std::optional<std::string> sourceId = JsonHelper::GetRawStringValueFromJsonNode(dataValue, JsonHelper::GetJsonKeyNameString(SourceIdentifier));
        if (!JsonHelper::IsValidNonEmptyStringValue(sourceId))
        {
            AICLI_LOG(Repo, Verbose, << "Missing source identifier");
            return {};
        }

        std::optional<std::reference_wrapper<const web::json::array>> versions = JsonHelper::GetRawJsonArrayFromJsonNode(dataValue, JsonHelper::GetJsonKeyNameString(ServerSupportedVersions));

        if (!versions.has_value() || versions.value().get().size() == 0)
        {
            AICLI_LOG(Repo, Verbose, << "Missing supported versions");
            return {};
        }

        std::vector<IRestClient::SupportedVersion> allVersions;
        for (auto& versionItem : versions.value().get())
        {
            std::optional<std::string> apiVersion = JsonHelper::GetRawStringValueFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(APIVersion));
            if (!JsonHelper::IsValidNonEmptyStringValue(apiVersion))
            {
                AICLI_LOG(Repo, Verbose, << "Missing API Version. Skipping version item.");
                continue;
            }

            std::optional<std::reference_wrapper<const web::json::array>> wingetVersions = JsonHelper::GetRawJsonArrayFromJsonNode(versionItem, JsonHelper::GetJsonKeyNameString(ObjectVersions));
            std::vector<std::string> supportedWingetVersions;
            if (wingetVersions.has_value())
            {
                for (auto& version : wingetVersions.value().get())
                {
                    std::optional<std::string> sp = JsonHelper::GetRawStringValueFromJsonValue(version);
                    if (sp.has_value())
                    {
                        supportedWingetVersions.emplace_back(std::move(sp.value()));
                    }
                }
            }

            if (supportedWingetVersions.size() == 0)
            {
                AICLI_LOG(Repo, Verbose, << "Missing supported winget versions. Skipping version item:" << apiVersion.value());
                continue;
            }

            IRestClient::SupportedVersion sv{ std::move(apiVersion.value()), std::move(supportedWingetVersions) };
            allVersions.emplace_back(std::move(sv));
        }

        if (allVersions.size() == 0)
        {
            AICLI_LOG(Repo, Verbose, << "Received incomplete information.");
            return {};
        }

        IRestClient::Information info{ std::move(sourceId.value()), std::move(allVersions) };
        return info;
    }
}
