// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "RestClient.h"
#include "Rest/Schema/1_0/Interface.h"

using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest
{
    RestClient::RestClient(const std::string& restApi)
    {
        // TODO: Ask for supported version from Rest API and Get version specific interface.
        m_interface = std::make_unique<Schema::V1_0::Interface>(restApi);
    }

    std::optional<Manifest::Manifest> RestClient::GetManifestByVersion(const std::string& packageId, const std::string& version, const std::string& channel) const
    {
        return m_interface->GetManifestByVersion(packageId, version, channel);
    }

    RestClient::SearchResult RestClient::Search(const SearchRequest& request) const
    {
        return m_interface->Search(request);
    }
}
