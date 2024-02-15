// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestRestRequestHandler.h"
#include <Rest/Schema/1_0/Interface.h>
#include <Rest/Schema/IRestClient.h>

std::shared_ptr<TestRestRequestHandler> GetTestRestRequestHandler(
    const web::http::status_code statusCode, const utility::string_t& sampleResponseString, const utility::string_t& mimeType)
{
    return std::make_shared<TestRestRequestHandler>([statusCode, sampleResponseString, mimeType](web::http::http_request request) ->
        pplx::task<web::http::http_response>
        {
            web::http::http_response response;
            if (sampleResponseString.empty())
            {
                response.set_body(utf16string{});
            }
            else
            {
                response.set_body(web::json::value::parse(sampleResponseString));
            }

            response.headers().set_content_type(mimeType);
            response.set_status_code(statusCode);
            return pplx::task_from_result(response);
        });
}

std::shared_ptr<TestRestRequestHandler> GetTestRestRequestHandler(
    std::function<web::http::status_code(const web::http::http_request& request)> handler)
{
    return std::make_shared<TestRestRequestHandler>([handler = std::move(handler)](web::http::http_request request) ->
        pplx::task<web::http::http_response>
        {
            web::http::http_response response;
            response.set_body(utf16string{});

            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.set_status_code(handler(request));
            return pplx::task_from_result(response);
        });
}

std::shared_ptr<TestRestRequestHandler> GetHeaderVerificationHandler(
    const web::http::status_code statusCode, const utility::string_t& sampleResponseString, const std::pair<utility::string_t, utility::string_t>& header, web::http::status_code statusCodeOnFailure)
{
    return std::make_shared<TestRestRequestHandler>([statusCode, sampleResponseString, header, statusCodeOnFailure](web::http::http_request request) ->
        pplx::task<web::http::http_response>
        {
            web::http::http_response response;
            auto& headers = request.headers();
            if (!headers.has(header.first) ||
                (utility::conversions::to_utf8string(header.second).compare(utility::conversions::to_utf8string(headers[header.first]))) != 0)
            {
                response.set_body(utf16string{ L"Expected header not found" });
                response.set_status_code(statusCodeOnFailure);
                return pplx::task_from_result(response);
            }

            if (!sampleResponseString.empty())
            {
                response.set_body(web::json::value::parse(sampleResponseString));
            }

            response.headers().set_content_type(web::http::details::mime_types::application_json);
            response.set_status_code(statusCode);
            return pplx::task_from_result(response);
        });
}