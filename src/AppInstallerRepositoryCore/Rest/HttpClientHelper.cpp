// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "HttpClientHelper.h"

namespace AppInstaller::Repository::Rest
{
    HttpClientHelper::HttpClientHelper(std::optional<std::shared_ptr<web::http::http_pipeline_stage>> stage) : m_defaultRequestHandlerStage(stage) {}

    pplx::task<web::http::http_response> HttpClientHelper::Post(
        const utility::string_t& uri, const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers) const
    {
        AICLI_LOG(Repo, Verbose, << "Sending http POST request to: " << utility::conversions::to_utf8string(uri));
        web::http::client::http_client client = GetClient(uri);
        web::http::http_request request{ web::http::methods::POST };
        request.headers().set_content_type(web::http::details::mime_types::application_json);
        request.set_body(body.serialize());

        // Add headers
        for (auto& pair : headers)
        {
            request.headers().add(pair.first, pair.second);
        }

        return client.request(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandlePost(
        const utility::string_t& uri, const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers) const
    {
        web::http::http_response httpResponse;
        HttpClientHelper::Post(uri, body, headers).then([&httpResponse](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());
                httpResponse = response;
            }).wait();

            return ValidateAndExtractResponse(httpResponse);
    }

    pplx::task<web::http::http_response> HttpClientHelper::Get(
        const utility::string_t& uri, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers) const
    {
        AICLI_LOG(Repo, Verbose, << "Sending http GET request to: " << utility::conversions::to_utf8string(uri));
        web::http::client::http_client client = GetClient(uri);
        web::http::http_request request{ web::http::methods::GET };
        request.headers().set_content_type(web::http::details::mime_types::application_json);

        // Add headers
        for (auto& pair : headers)
        {
            request.headers().add(pair.first, pair.second);
        }

        return client.request(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandleGet(
        const utility::string_t& uri, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers) const
    {
        web::http::http_response httpResponse;
        Get(uri, headers).then([&httpResponse](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());
                httpResponse = response;
            }).wait();

            return ValidateAndExtractResponse(httpResponse);
    }

    web::http::client::http_client HttpClientHelper::GetClient(const utility::string_t& uri) const
    {
        web::http::client::http_client client{ uri };

        // Add default custom handlers if any.
        if (m_defaultRequestHandlerStage)
        {
            client.add_handler(m_defaultRequestHandlerStage.value());
        }

        return client;
    }

    std::optional<web::json::value> HttpClientHelper::ValidateAndExtractResponse(const web::http::http_response& response) const
    {
        std::optional<web::json::value> result;
        switch (response.status_code())
        {
        case web::http::status_codes::OK:
            result = ExtractJsonResponse(response);
            break;

        case web::http::status_codes::NotFound:
            result = {};
            break;

        case web::http::status_codes::BadRequest:
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_INTERNAL_ERROR);
            break;

        default:
            THROW_HR(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, response.status_code()));
            break;
        }

        return result;
    }

    std::optional<web::json::value> HttpClientHelper::ExtractJsonResponse(const web::http::http_response& response) const
    {
        utility::string_t contentType = response.headers().content_type();

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE,
            !contentType._Starts_with(web::http::details::mime_types::application_json));

        return response.extract_json().get();
    }
}
