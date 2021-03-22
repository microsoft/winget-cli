// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"

namespace AppInstaller::Repository::Rest
{
    struct HttpClientHelper
    {
        HttpClientHelper(const utility::string_t& url);

        pplx::task<web::http::http_response> Post(const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {});

        web::json::value HandlePost(const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {});

        pplx::task<web::http::http_response> Get(const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {});

        web::json::value HandleGet(const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {});

    protected:
        pplx::task<web::http::http_response> MakeRequest(web::http::http_request req);

    private:
        web::http::client::http_client m_client;
        utility::string_t m_url;
    };
}
