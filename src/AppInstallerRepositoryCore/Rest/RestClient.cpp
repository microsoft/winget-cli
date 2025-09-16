// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/Schema/1_1/Interface.h"
#include "Rest/Schema/1_4/Interface.h"
#include "Rest/Schema/1_5/Interface.h"
#include "Rest/Schema/1_6/Interface.h"
#include "Rest/Schema/1_7/Interface.h"
#include "Rest/Schema/1_9/Interface.h"
#include "Rest/Schema/1_10/Interface.h"
#include "Rest/Schema/1_12/Interface.h"
#include "Rest/Schema/InformationResponseDeserializer.h"
#include "Rest/Schema/CommonRestConstants.h"
#include <winget/HttpClientHelper.h>
#include <winget/Rest.h>
#include <winget/JsonUtil.h>

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::V1_0;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Http;

namespace AppInstaller::Repository::Rest
{
    // Supported versions
    std::set<Version> WingetSupportedContracts = {
        Version_1_0_0,
        Version_1_1_0,
        Version_1_4_0,
        Version_1_5_0,
        Version_1_6_0,
        Version_1_7_0,
        Version_1_9_0,
        Version_1_10_0,
        Version_1_12_0,
    };

    constexpr std::string_view WindowsPackageManagerHeader = "Windows-Package-Manager"sv;
    constexpr size_t WindowsPackageManagerHeaderMaxLength = 1024;

    namespace
    {
        HttpClientHelper::HttpRequestHeaders GetHeaders(const std::optional<std::string>& customHeader, std::string_view caller)
        {
            HttpClientHelper::HttpRequestHeaders headers;

            if (customHeader)
            {
                AICLI_LOG(Repo, Verbose, << "Custom header found: " << customHeader.value());
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_CUSTOMHEADER_EXCEEDS_MAXLENGTH, customHeader.value().size() > WindowsPackageManagerHeaderMaxLength);
                headers.emplace(JSON::GetUtilityString(WindowsPackageManagerHeader), JSON::GetUtilityString(customHeader.value()));
            }

            if (!caller.empty())
            {
                AICLI_LOG(Repo, Verbose, << "User agent caller found: " << caller);
                std::wstring userAgentWide = JSON::GetUtilityString(Runtime::GetUserAgent(caller));
                try
                {
                    // Replace user profile if the caller binary is under user profile.
                    userAgentWide = Utility::ReplaceWhileCopying(userAgentWide, Runtime::GetPathTo(Runtime::PathName::UserProfile).wstring(), L"%USERPROFILE%");
                }
                CATCH_LOG();
                headers.emplace(web::http::header_names::user_agent, userAgentWide);
            }

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

    Schema::IRestClient::Information RestClient::GetInformation(const std::string& restApi, const std::optional<std::string>& customHeader, std::string_view caller, const HttpClientHelper& helper)
    {
        // Check the cache for a valid information entry
        RestInformationCache informationCache;
        std::optional<Schema::IRestClient::Information> cachedInformation = informationCache.Get(restApi, customHeader, caller);

        if (cachedInformation)
        {
            return std::move(cachedInformation).value();
        }

        // Not in cache, make REST call to retrieve it
        utility::string_t restEndpoint = AppInstaller::Rest::GetRestAPIBaseUri(restApi);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !AppInstaller::Rest::IsValidUri(restEndpoint));

        auto headers = GetHeaders(customHeader, caller);
        CacheControlPolicy cacheControl;

        utility::string_t endpoint = AppInstaller::Rest::AppendPathToUri(restEndpoint, JSON::GetUtilityString(InformationGetEndpoint));
        std::optional<web::json::value> response = helper.HandleGet(
            endpoint,
            headers,
            {},
            [&](const web::http::http_response& httpResponse)
            {
                // TODO: Extract cache time out
                // MaxAge [(in seconds) - Age == end of cache]
                // NoCache/NoStore [don't cache]
                // Public [ignore headers]
                cacheControl = CacheControlPolicy{ httpResponse.headers().cache_control() };
                return Http::HttpClientHelper::HttpResponseHandlerResult{ std::nullopt, true };
            });

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !response);

        InformationResponseDeserializer responseDeserializer;
        auto result = responseDeserializer.Deserialize(response.value());

        // Cache the information value as requested
        informationCache.Cache(restApi, customHeader, caller, cacheControl, response.value());

        return result;
    }

    std::unique_ptr<Schema::IRestClient> RestClient::GetSupportedInterface(
        const std::string& api,
        const HttpClientHelper::HttpRequestHeaders& additionalHeaders,
        const IRestClient::Information& information,
        const Authentication::AuthenticationArguments& authArgs,
        const Version& version,
        const HttpClientHelper& helper)
    {
        if (version == Version_1_0_0)
        {
            return std::make_unique<Schema::V1_0::Interface>(api, helper);
        }
        else if (version == Version_1_1_0)
        {
            return std::make_unique<Schema::V1_1::Interface>(api, helper, information, additionalHeaders);
        }
        else if (version == Version_1_4_0)
        {
            return std::make_unique<Schema::V1_4::Interface>(api, helper, information, additionalHeaders);
        }
        else if (version == Version_1_5_0)
        {
            return std::make_unique<Schema::V1_5::Interface>(api, helper, information, additionalHeaders);
        }
        else if (version == Version_1_6_0)
        {
            return std::make_unique<Schema::V1_6::Interface>(api, helper, information, additionalHeaders);
        }
        else if (version == Version_1_7_0)
        {
            return std::make_unique<Schema::V1_7::Interface>(api, helper, information, additionalHeaders, authArgs);
        }
        else if (version == Version_1_9_0)
        {
            return std::make_unique<Schema::V1_9::Interface>(api, helper, information, additionalHeaders, authArgs);
        }
        else if (version == Version_1_10_0)
        {
            return std::make_unique<Schema::V1_10::Interface>(api, helper, information, additionalHeaders, authArgs);
        }
        else if (version == Version_1_12_0)
        {
            return std::make_unique<Schema::V1_12::Interface>(api, helper, information, additionalHeaders, authArgs);
        }

        THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_VERSION);
    }

    RestClient RestClient::Create(
        const std::string& restApi,
        const std::optional<std::string>& customHeader,
        std::string_view caller,
        const HttpClientHelper& helper,
        const Schema::IRestClient::Information& information,
        const Authentication::AuthenticationArguments& authArgs)
    {
        utility::string_t restEndpoint = AppInstaller::Rest::GetRestAPIBaseUri(restApi);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_INVALID_URL, !AppInstaller::Rest::IsValidUri(restEndpoint));

        auto headers = GetHeaders(customHeader, caller);

        std::optional<Version> latestCommonVersion = GetLatestCommonVersion(information.ServerSupportedVersions, WingetSupportedContracts);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_UNSUPPORTED_RESTSOURCE, !latestCommonVersion);

        std::unique_ptr<Schema::IRestClient> supportedInterface = GetSupportedInterface(utility::conversions::to_utf8string(restEndpoint), headers, information, authArgs, latestCommonVersion.value(), helper);
        return RestClient{ std::move(supportedInterface), information.SourceIdentifier };
    }
}
