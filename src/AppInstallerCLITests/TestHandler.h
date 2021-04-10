// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "cpprest/http_client.h"

class TestHandler : public web::http::http_pipeline_stage
{
public:
    TestHandler(const std::function<pplx::task<web::http::http_response>(web::http::http_request request)>& handler) : m_handler(handler) {}

    virtual pplx::task<web::http::http_response> propagate(web::http::http_request request)
    {
        return m_handler(request);
    }

private:
    std::function<pplx::task<web::http::http_response>(web::http::http_request request)> m_handler;
};

std::shared_ptr<TestHandler> GetTestHandler(
    const web::http::status_code& statusCode, const utility::string_t& sampleResponseString = {}, const utility::string_t& mimeType = web::http::details::mime_types::application_json);
