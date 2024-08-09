// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Certificates.h>
#include <cpprest/http_client.h>
#include <cpprest/json.h>
#include <optional>
#include <vector>

namespace AppInstaller::Http
{
    struct HttpClientHelper
    {
        using HttpRequestHeaders = std::unordered_map<utility::string_t, utility::string_t>;

        HttpClientHelper(std::shared_ptr<web::http::http_pipeline_stage> = {});

        pplx::task<web::http::http_response> Post(const utility::string_t& uri, const web::json::value& body, const HttpRequestHeaders& headers = {}, const HttpRequestHeaders& authHeaders = {}) const;

        std::optional<web::json::value> HandlePost(const utility::string_t& uri, const web::json::value& body, const HttpRequestHeaders& headers = {}, const HttpRequestHeaders& authHeaders = {}) const;

        pplx::task<web::http::http_response> Get(const utility::string_t& uri, const HttpRequestHeaders& headers = {}, const HttpRequestHeaders& authHeaders = {}) const;

        std::optional<web::json::value> HandleGet(const utility::string_t& uri, const HttpRequestHeaders& headers = {}, const HttpRequestHeaders& authHeaders = {}) const;

        void SetPinningConfiguration(const Certificates::PinningConfiguration& configuration);

    protected:
        std::optional<web::json::value> ValidateAndExtractResponse(const web::http::http_response& response) const;

        std::optional<web::json::value> ExtractJsonResponse(const web::http::http_response& response) const;

    private:
        web::http::client::http_client GetClient(const utility::string_t& uri) const;

        std::shared_ptr<web::http::http_pipeline_stage> m_defaultRequestHandlerStage;
        web::http::client::http_client_config m_clientConfig;
    };
}
