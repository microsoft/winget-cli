// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_6/Interface.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/IRestClient.h"
#include <winget/HttpClientHelper.h>
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_6
{
    Interface::Interface(
        const std::string& restApi,
        const Http::HttpClientHelper& httpClientHelper,
        IRestClient::Information information,
        const Http::HttpClientHelper::HttpRequestHeaders& additionalHeaders) : V1_5::Interface(restApi, httpClientHelper, std::move(information), additionalHeaders)
    {
        m_requiredRestApiHeaders[JSON::GetUtilityString(ContractVersion)] = JSON::GetUtilityString(Version_1_6_0.ToString());
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_6_0;
    }
}
