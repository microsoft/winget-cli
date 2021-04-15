// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <cpprest/http_client.h>
#include <cpprest/json.h>

#include <optional>
#include <vector>

namespace AppInstaller::Repository::Rest
{
    struct HttpClientHelper
    {
        HttpClientHelper(std::optional<std::shared_ptr<web::http::http_pipeline_stage>> = {});

        pplx::task<web::http::http_response> Post(const utility::string_t& uri, const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {}) const;

        std::optional<web::json::value> HandlePost(const utility::string_t& uri, const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {}) const;

        pplx::task<web::http::http_response> Get(const utility::string_t& uri, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {}) const;

        std::optional<web::json::value> HandleGet(const utility::string_t& uri, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers = {}) const;
    
    protected:
        std::optional<web::json::value> ValidateAndExtractResponse(const web::http::http_response& response) const;

        std::optional<web::json::value> ExtractJsonResponse(const web::http::http_response& response) const;

    private:
        web::http::client::http_client GetClient(const utility::string_t& uri) const;

        std::optional<std::shared_ptr<web::http::http_pipeline_stage>> m_defaultRequestHandlerStage;
    };
}
