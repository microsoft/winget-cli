// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "IRestClient.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::client;
using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
    IRestClient::SearchResult Interface::Search(std::string m_restApiUri, const SearchRequest& request) const
    {
        UNREFERENCED_PARAMETER(request);

		SearchResult result = SearchResult();
		result.Matches = std::vector<IRestClient::Package>();

		// Call the ManifestSearch API and return a sample set of results.
		std::string searchEndPoint = m_restApiUri + "api/manifestSearch?";
		utility::string_t api = utility::conversions::to_string_t(searchEndPoint);

		json::value json_v;
		json_v[L"fetchAllManifests"] = web::json::value::string(L"true");
		http_client client(api);
		http_request req(methods::POST);
		req.headers().set_content_type(web::http::details::mime_types::application_json);
		req.set_body(json_v.serialize());

		client.request(req)
			.then([](const http_response& response)
				{
					try {
						std::cout << "Http response status returned: " << response.status_code() << "\n";
					}
					catch (const http_exception& e) {
						std::cout << "error " << e.what() << std::endl;
					}

					return response.extract_json().get();
				}).then([&result](json::value jsonObject)
					{
						// utility::string_t data = jsonObject.at(U("data")).as_string();
						// std::wcout << data;

						std::wcout << "Value: " << jsonObject.serialize() << std::endl;
						// Parse json and add results to SearchResult
						auto dataArray = jsonObject.at(U("data")).as_array();

						for (auto manifest : dataArray)
						{
							// result.Matches.emplace_back(utility::conversions::to_utf8string(manifest.as_string()));
						}
					}).wait();

					return result;
    }

    // TODO: Pass version
    std::optional<std::string> Interface::GetVersionFromPackage(const Manifest::Manifest& manifest, std::string_view version, std::string_view channel) const
    {
        UNREFERENCED_PARAMETER(manifest);
        UNREFERENCED_PARAMETER(version);
        UNREFERENCED_PARAMETER(channel);
        return {};
    }

    std::vector<Utility::VersionAndChannel> Interface::GetVersionKeysFromPackage(const Manifest::Manifest& manifest) const
    {
        UNREFERENCED_PARAMETER(manifest);
        std::vector<Utility::VersionAndChannel> result;
        return result;
    }

    // TODO: Pass version
    std::optional<std::string> Interface::GetPropertyFromVersion(const std::string& manifest, PackageVersionProperty property) const
    {
        UNREFERENCED_PARAMETER(manifest);
        UNREFERENCED_PARAMETER(property);
        return {};
    }
}
