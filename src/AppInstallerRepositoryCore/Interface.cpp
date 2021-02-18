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
		const std::basic_string FetchAllManifests = L"fetchAllManifests";

		json::value GetSearchBody(const SearchRequest& searchRequest)
		{
			// TODO: Use search request to construct search body.
			UNREFERENCED_PARAMETER(searchRequest);

			json::value json_body;
			json_body[FetchAllManifests] = web::json::value::string(L"true");

			return json_body;
		}
	}

	IRestClient::SearchResult Interface::Search(std::string m_restApiUri, const SearchRequest& request) const
	{
		UNREFERENCED_PARAMETER(request);

		SearchResult result;

		// Call the ManifestSearch API and return a sample set of results.
		std::string fullSearchAPI = m_restApiUri + "api/manifestSearch?";;
		utility::string_t searchAPI = utility::conversions::to_string_t(fullSearchAPI);
		
		HttpClientHelper clientHelper(searchAPI);
		json::value requestBody = GetSearchBody(request);
		json::value jsonObject = clientHelper.Handle_Post(requestBody);

		// Parse json and add results to SearchResult
		auto& dataArray = jsonObject.at(U("data")).as_array();

		for (auto& manifestItem : dataArray)
		{
			// Deserialize to manifest object. 
			std::wcout << manifestItem.serialize();
			/*Manifest::Manifest manifest = Manifest::YamlParser::Create(utility::conversions::to_utf8string(manifestItem.serialize()));
			Package package = Package(std::move(manifest));
			result.Matches.emplace_back(std::move(package));*/
		}

		return result;
	}

	std::optional<std::string> Interface::GetManifestByVersion(std::string m_restApiUri, std::string packageId, std::string version) const
	{
		// Call the Version API and get the manifest corresponding to package id and version.
		std::string versionAPI = m_restApiUri + "api/packages/" + packageId + "/versions/" + version;
		utility::string_t versionApi = utility::conversions::to_string_t(versionAPI);

		HttpClientHelper clientHelper(versionApi);
		json::value jsonObject = clientHelper.Handle_Get();

		// Parse json and add results to SearchResult
		auto& dataArray = jsonObject.at(U("data")).as_array();
		std::string manifest;
		for (auto& manifestItem : dataArray)
		{
			manifest = utility::conversions::to_utf8string(manifestItem.serialize());
		}

		return manifest;
	}

	// TODO: Pass version
	std::optional<std::string> Interface::GetVersionFromPackage(const Manifest::Manifest& manifest, std::string version, std::string channel) const
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
