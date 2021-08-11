// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/1_1/Interface.h"
#include "Rest/Schema/HttpClientHelper.h"
#include "Rest/Schema/InformationResponseDeserializer.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/RestHelper.h"

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Rest
{
    // Supported versions
    std::set<Version> WingetSupportedContracts = { Version_1_0_0, Version_1_1_0 };

    RestClient::RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface, std::string sourceIdentifier, IRestClient::Information&& information)
        : m_interface(std::move(supportedInterface)), m_sourceIdentifier(std::move(sourceIdentifier)), m_information(std::move(information))
    {
    }

    std::optional<Manifest::Manifest> RestClient::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        return m_interface->GetManifestByVersion(packageId, version, channel);
    }

    RestClient::SearchResult RestClient::Search(const SearchRequest& request) const
    {
        return m_interface->Search(request);
    }

    std::string RestClient::GetSourceIdentifier() const
    {
        return m_sourceIdentifier;
    }

    IRestClient::Information RestClient::GetInformation(const utility::string_t& restApi, const HttpClientHelper& clientHelper)
    {
        // Call information endpoint
        utility::string_t endpoint = RestHelper::AppendPathToUri(restApi, JsonHelper::GetUtilityString(InformationGetEndpoint));
        std::optional<web::json::value> response = clientHelper.HandleGet(endpoint);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !response);

        InformationResponseDeserializer responseDeserializer;
        IRestClient::Information information = responseDeserializer.Deserialize(response.value());

        return information;
    }

    std::optional<Version> RestClient::GetLatestCommonVersion(
        const std::vector<std::string>& serverSupportedVersions,
        const std::set<Version>& wingetSupportedVersions)
    {
        std::set<Version> commonVersions;
        for (auto& version : serverSupportedVersions)
        {
            Version versionInfo(version);
            if (wingetSupportedVersions.find(versionInfo) != wingetSupportedVersions.end())
            {
                commonVersions.insert(std::move(versionInfo));
            }
        }

        if (commonVersions.empty())
        {
            return {};
        }

        return *commonVersions.rbegin();
    }

    std::unique_ptr<Schema::IRestClient> RestClient::GetSupportedInterface(const std::string& api, const IRestClient::Information& information, const Version& version)
    {
        if (version == Version_1_0_0)
        {
            return std::make_unique<Schema::V1_0::Interface>(api);
        }
        else if (version == Version_1_1_0)
        {
            return std::make_unique<Schema::V1_1::Interface>(api, information);
        }

        THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION);
    }

    RestClient RestClient::Create(const std::string& restApi, const HttpClientHelper& helper)
    {
        utility::string_t restEndpoint = RestHelper::GetRestAPIBaseUri(restApi);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !RestHelper::IsValidUri(restEndpoint));

        IRestClient::Information information = GetInformation(restEndpoint, helper);
        std::optional<Version> latestCommonVersion = GetLatestCommonVersion(information.ServerSupportedVersions, WingetSupportedContracts);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !latestCommonVersion);

        std::unique_ptr<Schema::IRestClient> supportedInterface = GetSupportedInterface(utility::conversions::to_utf8string(restEndpoint), information, latestCommonVersion.value());
        return RestClient{ std::move(supportedInterface), information.SourceIdentifier, std::move(information) };
    }
}
