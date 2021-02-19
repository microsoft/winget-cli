// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Interface.h"

using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest
{
	RestClient::RestClient(const std::string restApi) : m_restApiUri(restApi)
	{
		// TODO: Ask for supported version from Rest API and Get version specific interface.
		m_interface = std::make_unique<Schema::V1_0::Interface>();
	}

	std::optional<std::string> RestClient::GetManifestByVersion(const std::string& packageId, const std::string& version) const
	{
		return m_interface->GetManifestByVersion(m_restApiUri, packageId, version);
	}

	RestClient::SearchResult RestClient::Search(const SearchRequest& request) const
	{
		return m_interface->Search(m_restApiUri, request);
	}
}