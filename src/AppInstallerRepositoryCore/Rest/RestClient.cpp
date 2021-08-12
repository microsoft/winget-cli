// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/HttpClientHelper.h"
#include "Rest/Schema/1_0/Json/InformationResponseDeserializer.h"
#include "Rest/Schema/JsonHelper.h"
#include "Rest/Schema/1_0/Json/CommonJsonConstants.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/RestHelper.h"

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;
using namespace AppInstaller::Repository::Rest::Schema::V1_0::Json;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Rest
{
    // Supported versions
    std::set<Version> WingetSupportedContracts = { Version_1_0_0 };

    constexpr std::string_view WindowsPackageManagerHeader = "Windows-Package-Manager"sv;

    RestClient::RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface, std::string sourceIdentifier)
        : m_interface(std::move(supportedInterface)), m_sourceIdentifier(std::move(sourceIdentifier))
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

    utility::string_t RestClient::GetInformationEndpoint(const utility::string_t& restApiUri)
    {
        utility::string_t endpoint = RestHelper::AppendPathToUri(restApiUri, JsonHelper::GetUtilityString(InformationGetEndpoint));
        return endpoint;
    }

    IRestClient::Information RestClient::GetInformation(
        const utility::string_t& restApi, const std::unordered_map<utility::string_t, utility::string_t>& additionalHeaders, const HttpClientHelper& clientHelper)
    {
        // Call information endpoint
        std::optional<web::json::value> response = clientHelper.HandleGet(GetInformationEndpoint(restApi), additionalHeaders);

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !response);

        Json::InformationResponseDeserializer responseDeserializer;
        IRestClient::Information information = responseDeserializer.Deserialize(response.value());

        return information;
    }

    std::optional<Version> RestClient::GetLatestCommonVersion(
        const IRestClient::Information& information, const std::set<Version>& wingetSupportedVersions)
    {
        std::set<Version> commonVersions;
        for (auto& version : information.ServerSupportedVersions)
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

    std::unique_ptr<Schema::IRestClient> RestClient::GetSupportedInterface(
        const std::string& api, const std::unordered_map<utility::string_t, utility::string_t>& additionalHeaders, const Version& version)
    {
        if (version == Version_1_0_0)
        {
            return std::make_unique<Schema::V1_0::Interface>(api, additionalHeaders);
        }
       
        THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION);
    }

    RestClient RestClient::Create(const std::string& restApi, const AdditionalSourceData& sourceSettings, const HttpClientHelper& helper)
    {
        utility::string_t restEndpoint = RestHelper::GetRestAPIBaseUri(restApi);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !RestHelper::IsValidUri(restEndpoint));

        auto headers = GetHeaders(sourceSettings);

        IRestClient::Information information = GetInformation(restEndpoint, headers, helper);
        std::optional<Version> latestCommonVersion = GetLatestCommonVersion(information, WingetSupportedContracts);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !latestCommonVersion);

        std::unique_ptr<Schema::IRestClient> supportedInterface = GetSupportedInterface(utility::conversions::to_utf8string(restEndpoint), headers, latestCommonVersion.value());
        return RestClient{ std::move(supportedInterface), information.SourceIdentifier };
    }

    std::unordered_map<utility::string_t, utility::string_t> RestClient::GetHeaders(const AdditionalSourceData& sourceData)
    {
        if (sourceData.Header.empty())
        {
            AICLI_LOG(Repo, Verbose, << "No additional source data found.");
            return {};
        }

        std::unordered_map<utility::string_t, utility::string_t> headers;
        headers.emplace(JsonHelper::GetUtilityString(WindowsPackageManagerHeader), JsonHelper::GetUtilityString(sourceData.Header));
        return headers;
    }
}
