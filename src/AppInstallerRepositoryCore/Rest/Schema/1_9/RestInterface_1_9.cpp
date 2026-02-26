// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Rest/Schema/1_9/Interface.h"
#include "Rest/Schema/CommonRestConstants.h"
#include "Rest/Schema/IRestClient.h"
#include <winget/HttpClientHelper.h>
#include <winget/JsonUtil.h>

namespace AppInstaller::Repository::Rest::Schema::V1_9
{
    Interface::Interface(
        const std::string& restApi,
        const Http::HttpClientHelper& httpClientHelper,
        IRestClient::Information information,
        const Http::HttpClientHelper::HttpRequestHeaders& additionalHeaders,
        Authentication::AuthenticationArguments authArgs) : V1_7::Interface(restApi, httpClientHelper, std::move(information), additionalHeaders, std::move(authArgs))
    {
        m_requiredRestApiHeaders[JSON::GetUtilityString(ContractVersion)] = JSON::GetUtilityString(Version_1_9_0.ToString());
    }

    Utility::Version Interface::GetVersion() const
    {
        return Version_1_9_0;
    }
}
