// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/JsonHelper.h"
#include "InformationResponseDeserializer.h"
#include "CommonJsonConstants.h"

namespace AppInstaller::Repository::Rest::Schema::V1_0::Json
{
    namespace
    {
        // Information response constants
        constexpr std::string_view SourceIdentifier = "SourceIdentifier"sv;
        constexpr std::string_view ServerSupportedVersions = "ServerSupportedVersions"sv;
    }

    IRestClient::Information InformationResponseDeserializer::Deserialize(const web::json::value& dataObject) const
    {
        // Get information result from json output.
        std::optional<IRestClient::Information> information = DeserializeInformation(dataObject);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !information);

        return information.value();
    }

    std::optional<IRestClient::Information> InformationResponseDeserializer::DeserializeInformation(const web::json::value& dataObject) const
    {
        try
        {
            if (dataObject.is_null())
            {
                AICLI_LOG(Repo, Error, << "Missing json object.");
                return {};
            }

            std::optional<std::reference_wrapper<const web::json::value>> data = JsonHelper::GetJsonValueFromNode(dataObject, JsonHelper::GetUtilityString(Data));
            if (!data)
            {
                AICLI_LOG(Repo, Error, << "Missing data");
                return {};
            }

            auto& dataValue = data.value().get();
            std::optional<std::string> sourceId = JsonHelper::GetRawStringValueFromJsonNode(dataValue, JsonHelper::GetUtilityString(SourceIdentifier));
            if (!JsonHelper::IsValidNonEmptyStringValue(sourceId))
            {
                AICLI_LOG(Repo, Error, << "Missing source identifier");
                return {};
            }

            std::optional<std::reference_wrapper<const web::json::array>> versions = JsonHelper::GetRawJsonArrayFromJsonNode(dataValue, JsonHelper::GetUtilityString(ServerSupportedVersions));

            if (!versions || versions.value().get().size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Missing supported versions");
                return {};
            }

            std::vector<std::string> allVersions;
            for (auto& versionItem : versions.value().get())
            {
                std::optional<std::string> sp = JsonHelper::GetRawStringValueFromJsonValue(versionItem);
                if (sp)
                {
                    allVersions.emplace_back(std::move(sp.value()));
                }
            }

            if (allVersions.size() == 0)
            {
                AICLI_LOG(Repo, Error, << "Received incomplete information.");
                return {};
            }

            IRestClient::Information info{ std::move(sourceId.value()), std::move(allVersions) };
            return info;
        }
        catch (...)
        {
            AICLI_LOG(Repo, Error, << "Received invalid information.");
        }

        return {};
    }
}
