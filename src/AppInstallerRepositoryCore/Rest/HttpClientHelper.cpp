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

    web::json::value HttpClientHelper::HandlePost(
        const web::json::value& body, const std::vector<std::pair<utility::string_t, utility::string_t>>& headers)
    {
        web::http::http_response httpResponse;
        HttpClientHelper::Post(body, headers).then([&httpResponse](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());
                httpResponse = response;
            }).wait();

            THROW_HR_IF(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, httpResponse.status_code()), httpResponse.status_code() != web::http::status_codes::OK);
            return httpResponse.extract_json().get();
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

    web::json::value HttpClientHelper::HandleGet(const std::vector<std::pair<utility::string_t, utility::string_t>>& headers)
    {
        web::http::http_response httpResponse;
        Get(headers).then([&httpResponse](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());
                httpResponse = response;
            }).wait();

            THROW_HR_IF(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, httpResponse.status_code()), httpResponse.status_code() != web::http::status_codes::OK);
            return httpResponse.extract_json().get();
    }

    pplx::task<web::http::http_response> HttpClientHelper::MakeRequest(web::http::http_request req)
    {
        return m_client.request(req);
    }
}
