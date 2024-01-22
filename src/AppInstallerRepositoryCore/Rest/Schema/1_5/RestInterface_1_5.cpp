// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_5/Interface.h"
#include "Rest/Schema/IRestClient.h"
#include "Rest/Schema/HttpClientHelper.h"
#include "Rest/Schema/CommonRestConstants.h"
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_5
{
    Interface::Interface(
        const std::string& restApi,
        IRestClient::Information information,
        const HttpClientHelper::HttpRequestHeaders& additionalHeaders,
        const HttpClientHelper& httpClientHelper) : V1_4::Interface(restApi, std::move(information), additionalHeaders, httpClientHelper)
    {
        m_requiredRestApiHeaders[JSON::GetUtilityString(ContractVersion)] = JSON::GetUtilityString(Version_1_5_0.ToString());
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_5_0;
    }
}
