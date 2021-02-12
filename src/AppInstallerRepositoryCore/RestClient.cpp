// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include <winget/ManifestYamlParser.h>
#include "cpprest/http_client.h"

namespace AppInstaller::Repository::Rest
{
	RestClient::RestClient(const std::string restApi) : m_restApiUri(restApi)
	{
	}

	RestClient::SearchResult RestClient::Search(const SearchRequest& request) const
	{
		UNREFERENCED_PARAMETER(request);
		SearchResult result = SearchResult();

		// Call the ManifestSearch API and return a sample set of results.
		std::string searchEndPoint = m_restApiUri + "api/manifestSearch?";
		utility::string_t api = utility::conversions::to_string_t(searchEndPoint);

		web::http::client::http_client client(api);
		client.request(web::http::methods::POST, U("/"))
			.then([](const web::http::http_response& response) {
			try {
				std::cout << response.status_code();
			}
			catch (const web::http::http_exception& e) {
				std::cout << "error " << e.what() << std::endl;
			}
				}).wait();

		return result;
	}
}