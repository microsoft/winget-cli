// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "HttpClientHelper.h"

namespace AppInstaller::Repository::Rest
{
    HttpClientHelper::HttpClientHelper(const utility::string_t& url) : m_client(url), m_url(url) {}

    pplx::task<web::http::http_response> HttpClientHelper::Post(const web::json::value& body)
    {
        AICLI_LOG(Repo, Verbose, << "Sending http POST request to: " << utility::conversions::to_utf8string(m_url));
        web::http::http_request request{ web::http::methods::POST };
        request.headers().set_content_type(web::http::details::mime_types::application_json);
        request.set_body(body.serialize());
        return MakeRequest(request);
    }

    web::json::value HttpClientHelper::HandlePost(const web::json::value& body)
    {
        web::json::value result;
        HttpClientHelper::Post(body).then([&result](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());

                if (response.status_code() == web::http::status_codes::OK)
                {
                    result = response.extract_json().get();
                }
            }).wait();

        return result;
    }

    pplx::task<web::http::http_response> HttpClientHelper::Get()
    {
        AICLI_LOG(Repo, Verbose, << "Sending http GET request to: " << utility::conversions::to_utf8string(m_url));
        web::http::http_request request{ web::http::methods::GET };
        request.headers().set_content_type(web::http::details::mime_types::application_json);
        return MakeRequest(request);
    }

    web::json::value HttpClientHelper::HandleGet()
    {
        web::json::value result;
        Get().then([&result](const web::http::http_response& response)
            {
                AICLI_LOG(Repo, Verbose, << "Response status: " << response.status_code());

                if (response.status_code() == web::http::status_codes::OK)
                {
                    result = response.extract_json().get();
                }

            }).wait();

        return result;
    }

    pplx::task<web::http::http_response> HttpClientHelper::MakeRequest(web::http::http_request req)
    {
        return m_client.request(req);
    }
}
