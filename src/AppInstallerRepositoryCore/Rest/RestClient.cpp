// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Rest/Schema/1_0/Interface.h"
#include "Rest/HttpClientHelper.h"
#include "Rest/Schema/Json/InformationResponseDeserializer.h"
#include "Rest/Schema/Json/JsonHelper.h"
#include "Rest/Schema/Json/CommonRestConstants.h"
#include "Rest/Schema/RestHelper.h"

using namespace AppInstaller::Repository::Rest::Schema;
using namespace AppInstaller::Repository::Rest::Schema::Json;

namespace AppInstaller::Repository::Rest
{
    RestClient::RestClient(std::unique_ptr<Schema::IRestClient> supportedInterface)
        : m_interface(std::move(supportedInterface))
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

    utility::string_t RestClient::GetInformationEndpoint(const std::string& restApiUri)
    {
        std::string informationApi = RestHelper::GetRestAPIBaseUri(restApiUri);
        return utility::conversions::to_string_t(informationApi.append(InformationGetEndpoint));
    }

    std::string RestClient::GetSupportedVersion(const std::string& restApi)
    {
        // Call information endpoint
        HttpClientHelper httpClientHelper{ GetInformationEndpoint(restApi) };
        web::json::value response = httpClientHelper.HandleGet();

        Json::InformationResponseDeserializer responseDeserializer;
        IRestClient::Information info = responseDeserializer.Deserialize(response);

        // TODO: Get a version that winget client and rest source both support. Using first version given for now.
        IRestClient::Information information{ std::move(info) };
        return information.ServerSupportedVersions[0];
    }

    std::unique_ptr<Schema::IRestClient> RestClient::GetSupportedInterface(const std::string& api, const std::string& version)
    {
        // TODO: Add supported version logic. Use V1_0 for now.
        UNREFERENCED_PARAMETER(version);
        return std::make_unique<Schema::V1_0::Interface>(api);
    }

    RestClient RestClient::Create(const std::string& restApi)
    {
        std::string version = GetSupportedVersion(restApi);
        std::unique_ptr<Schema::IRestClient> supportedInterface = GetSupportedInterface(restApi, version);
        return RestClient{ std::move(supportedInterface) };
    }
}
