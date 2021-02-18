// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "IRestClient.h"

namespace AppInstaller::Repository::Rest
{
	struct RestClient
	{
		RestClient(const std::string restApi);

		// The return type of Search
		using SearchResult = Rest::Schema::IRestClient::SearchResult;

		RestClient(const RestClient&) = delete;
		RestClient& operator=(const RestClient&) = delete;

		RestClient(RestClient&&) = default;
		RestClient& operator=(RestClient&&) = default;

		// Performs a search based on the given criteria.
		Schema::IRestClient::SearchResult Search(const SearchRequest& request) const;

		// Gets all versions and channels for the given id.
		std::vector<Utility::VersionAndChannel> GetVersionKeysFromPackage(const Manifest::Manifest& manifest) const;
				
		std::optional<std::string> GetManifestByVersion(std::string packageId, std::string version) const;

		std::optional<std::string> GetVersionFromPackage(const Manifest::Manifest& manifest, std::string version, std::string channel) const;

		std::optional<std::string> GetPropertyFromVersion(const std::string& manifest, PackageVersionProperty property) const;

		std::vector<std::string> GetMultiPropertyFromVersion(const std::string& manifest, PackageVersionMultiProperty packageVersionProperty) const;

	private:
		std::string m_restApiUri;
		std::unique_ptr<Schema::IRestClient> m_interface;
	};
}