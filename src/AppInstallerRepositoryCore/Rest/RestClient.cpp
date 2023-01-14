// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/1_1/Interface.h"
#include "Rest/Schema/1_4/Interface.h"
#include "Rest/Schema/HttpClientHelper.h"
#include <winget/JsonUtil.h>
#include "Rest/Schema/InformationResponseDeserializer.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/RestHelper.h"

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Rest
{
    // Supported versions
    std::set<Version> WingetSupportedContracts = { Version_1_0_0, Version_1_1_0, Version_1_4_0 };

    constexpr std::string_view WindowsPackageManagerHeader = "Windows-Package-Manager"sv;
    constexpr size_t WindowsPackageManagerHeaderMaxLength = 1024;

    namespace {
        std::unordered_map<utility::string_t, utility::string_t> GetHeaders(std::optional<std::string> customHeader)
        {
            if (!customHeader)
            {
                AICLI_LOG(Repo, Verbose, << "Custom header not found.");
                return {};
            }

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH, customHeader.value().size() > WindowsPackageManagerHeaderMaxLength);

            std::unordered_map<utility::string_t, utility::string_t> headers;
            headers.emplace(JSON::GetUtilityString(WindowsPackageManagerHeader), JSON::GetUtilityString(customHeader.value()));
            return headers;
        }
    }

    RestClient::RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface, std::string sourceIdentifier)
        : m_interface(std::move(supportedInterface)), m_sourceIdentifier(std::move(sourceIdentifier))
    {
    }

    std::optional<Manifest::Manifest> RestClient::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        return m_interface->GetManifestByVersion(packageId, version, channel);
    }

    IRestClient::SearchResult RestClient::Search(const SearchRequest& request) const
    {
        return m_interface->Search(request);
    }

    std::string RestClient::GetSourceIdentifier() const
    {
        return m_sourceIdentifier;
    }

    IRestClient::Information RestClient::GetSourceInformation() const
    {
        return m_interface->GetSourceInformation();
    }

    IRestClient::Information RestClient::GetInformation(
        const utility::string_t& restApi, const std::unordered_map<utility::string_t, utility::string_t>& additionalHeaders, const HttpClientHelper& clientHelper)
    {
        // Call information endpoint
        utility::string_t endpoint = RestHelper::AppendPathToUri(restApi, JSON::GetUtilityString(InformationGetEndpoint));
        std::optional<web::json::value> response = clientHelper.HandleGet(endpoint, additionalHeaders);

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
            auto itr = std::find_if(wingetSupportedVersions.begin(), wingetSupportedVersions.end(),
                [&](const Version& v)
                {
                    // Only check major and minor version match if applicable
                    if (v.GetParts().size() >= 2)
                    {
                        return versionInfo.GetParts().size() >= 2 &&
                            versionInfo.GetParts().at(0) == v.GetParts().at(0) &&
                            versionInfo.GetParts().at(1) == v.GetParts().at(1);
                    }
                    else
                    {
                        return versionInfo == v;
                    }
                });
            if (itr != wingetSupportedVersions.end())
            {
                commonVersions.insert(*itr);
            }
        }

        if (commonVersions.empty())
        {
            return {};
        }

        return *commonVersions.rbegin();
    }

    std::unique_ptr<Schema::IRestClient> RestClient::GetSupportedInterface(
        const std::string& api,
        const std::unordered_map<utility::string_t, utility::string_t>& additionalHeaders,
        const IRestClient::Information& information,
        const Version& version)
    {
        if (version == Version_1_0_0)
        {
            return std::make_unique<Schema::V1_0::Interface>(api);
        }
        else if (version == Version_1_1_0)
        {
            return std::make_unique<Schema::V1_1::Interface>(api, information, additionalHeaders);
        }
        else if (version == Version_1_4_0)
        {
            return std::make_unique<Schema::V1_4::Interface>(api, information, additionalHeaders);
        }

        THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION);
    }

    RestClient RestClient::Create(const std::string& restApi, std::optional<std::string> customHeader, const HttpClientHelper& helper)
    {
        utility::string_t restEndpoint = RestHelper::GetRestAPIBaseUri(restApi);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !RestHelper::IsValidUri(restEndpoint));

        auto headers = GetHeaders(customHeader);

        IRestClient::Information information = GetInformation(restEndpoint, headers, helper);
        std::optional<Version> latestCommonVersion = GetLatestCommonVersion(information.ServerSupportedVersions, WingetSupportedContracts);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !latestCommonVersion);

        std::unique_ptr<Schema::IRestClient> supportedInterface = GetSupportedInterface(utility::conversions::to_utf8string(restEndpoint), headers, information, latestCommonVersion.value());
        return RestClient{ std::move(supportedInterface), information.SourceIdentifier };
    }
}
