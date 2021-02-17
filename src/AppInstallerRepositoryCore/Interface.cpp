// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Interface.h"
#include "IRestClient.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "HttpClientHelper.h"
#include <winget/ManifestYamlParser.h>

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::client;
using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest::Schema::V1_0
{
	namespace
	{
		json::value GetSearchBody(const SearchRequest& searchRequest)
		{
			UNREFERENCED_PARAMETER(searchRequest);

			json::value json_body;
			json_body[L"fetchAllManifests"] = web::json::value::string(L"true");

			return json_body;
		}
	}

	IRestClient::SearchResult Interface::Search(std::string m_restApiUri, const SearchRequest& request) const
	{
		UNREFERENCED_PARAMETER(request);

		SearchResult result;

		// Call the ManifestSearch API and return a sample set of results.
		utility::string_t searchAPI = utility::conversions::to_string_t(m_restApiUri);
		
		HttpClientHelper clientHelper(searchAPI);
		json::value requestBody = GetSearchBody(request);
		json::value jsonObject = clientHelper.Handle_Post(requestBody);

		// Parse json and add results to SearchResult
		auto& dataArray = jsonObject.at(U("data")).as_array();

		for (auto& manifestItem : dataArray)
		{
			// Deserialize to manifest object. 
			Manifest::Manifest manifest = Manifest::YamlParser::Create(utility::conversions::to_utf8string(manifestItem.serialize()));
			Package package = Package(std::move(manifest));
			result.Matches.emplace_back(std::move(package));
		}

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

	// TODO: Pass version
	std::vector<std::string> Interface::GetMultiPropertyFromVersion(const std::string& manifest, PackageVersionMultiProperty property) const
	{
		UNREFERENCED_PARAMETER(manifest);
		UNREFERENCED_PARAMETER(property);
		std::vector<std::string> result;
		return result;
	}
}
