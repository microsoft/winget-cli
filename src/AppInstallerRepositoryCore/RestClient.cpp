// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include <winget/ManifestYamlParser.h>
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include <Interface.h>
#include "IRestClient.h"

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::client;
using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest
{
	using namespace std::chrono_literals;
	using namespace std::string_view_literals;

	const std::string searchAPI = "api/manifestSearch?";

	RestClient::RestClient(const std::string restApi) : m_restApiUri(restApi)
	{
		// TODO: Ask for supported version from server
		// Get version specific interface. Ex: V1
		m_interface = std::make_unique<Schema::V1_0::Interface>();
	}

	std::vector<Utility::VersionAndChannel> RestClient::GetVersionKeysFromPackage(const Manifest::Manifest& manifest) const
	{
		return m_interface->GetVersionKeysFromPackage(manifest);
	}

	std::optional<std::string> RestClient::GetVersionFromPackage(const Manifest::Manifest& manifest, std::string_view version, std::string_view channel) const
	{
		return m_interface->GetVersionFromPackage(manifest, version, channel);
	}

	std::optional<std::string> RestClient::GetPropertyFromVersion(const std::string& manifest, PackageVersionProperty packageVersionProperty) const
	{
		return m_interface->GetPropertyFromVersion(manifest, packageVersionProperty);
	}

	std::vector<std::string> RestClient::GetMultiPropertyFromVersion(const std::string& manifest, PackageVersionMultiProperty packageVersionProperty) const
	{
		return m_interface->GetMultiPropertyFromVersion(manifest, packageVersionProperty);
	}

	RestClient::SearchResult RestClient::Search(const SearchRequest& request) const
	{
		// Call search from version specific interface.
		std::string fullSearchAPI = m_restApiUri + searchAPI;
		return m_interface->Search(fullSearchAPI, request);
	}
}