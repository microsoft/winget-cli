// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "HttpClientHelper.h"

namespace AppInstaller::Repository::Rest
{
    HttpClientHelper::HttpClientHelper(const utility::string_t& url) : m_client(url), m_url(url) {}

    pplx::task<web::http::http_response> HttpClientHelper::Post(
        const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers)
    {
        AICLI_LOG(Repo, Verbose, << "Sending http POST request to: " << utility::conversions::to_utf8string(m_url));
        web::http::http_request request{ web::http::methods::POST };
        request.headers().set_content_type(web::http::details::mime_types::application_json);
        request.set_body(body.serialize());

        // Add headers
        for (auto& pair : headers)
        {
            request.headers().add(pair.first, pair.second);
        }

        return MakeRequest(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandlePost(
        const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers)
    {
        web::http::http_response httpResponse;
        HttpClientHelper::Post(body, headers).then([&httpResponse](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());
                httpResponse = response;
            }).wait();

            return ValidateAndExtractResponse(httpResponse);
    }

    pplx::task<web::http::http_response> HttpClientHelper::Get(const std::vector<std::pair<utility::string_t, utility::string_t>>& headers)
    {
        AICLI_LOG(Repo, Verbose, << "Sending http GET request to: " << utility::conversions::to_utf8string(m_url));
        web::http::http_request request{ web::http::methods::GET };
        request.headers().set_content_type(web::http::details::mime_types::application_json);

        // Add headers
        for (auto& pair : headers)
        {
            request.headers().add(pair.first, pair.second);
        }

        return MakeRequest(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandleGet(const std::vector<std::pair<utility::string_t, utility::string_t>>& headers)
    {
        web::http::http_response httpResponse;
        Get(headers).then([&httpResponse](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());
                httpResponse = response;
            }).wait();

            return ValidateAndExtractResponse(httpResponse);
    }

    std::optional<web::json::value> HttpClientHelper::ValidateAndExtractResponse(const web::http::http_response& response)
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

    std::optional<web::json::value> HttpClientHelper::ExtractJsonResponse(const web::http::http_response& response)
    {
        utility::string_t contentType = response.headers().content_type();

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTSOURCE_UNSUPPORTED_MIME_TYPE,
            !contentType._Starts_with(web::http::details::mime_types::application_json));

        return response.extract_json().get();
    }

    pplx::task<web::http::http_response> HttpClientHelper::MakeRequest(web::http::http_request req)
    {
        return m_client.request(req);
    }
}
