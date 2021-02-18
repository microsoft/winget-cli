// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Interface.h"
#include "IRestClient.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"
#include "HttpClientHelper.h"

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::client;
using namespace AppInstaller::Repository::Rest::Schema;

namespace AppInstaller::Repository::Rest
{
	HttpClientHelper::HttpClientHelper(utility::string_t url) : m_client(url), m_url(url) {}

	pplx::task<web::http::http_response> HttpClientHelper::Post(const json::value& body)
	{
		http_request request(methods::POST);
		request.headers().set_content_type(web::http::details::mime_types::application_json);
		request.set_body(body.serialize());
		return MakeRequest(request);
	}

	json::value HttpClientHelper::HandlePost(const json::value& body)
	{
		json::value result;
		Post(body).then([&result](const http_response& response)
			{
				if (response.status_code() == status_codes::OK)
				{
					result = response.extract_json().get();
				}

			}).wait();

		return result;
	}

	pplx::task<web::http::http_response> HttpClientHelper::Get()
	{
		http_request request(methods::GET);
		request.headers().set_content_type(web::http::details::mime_types::application_json);
		return MakeRequest(request);
	}

	json::value HttpClientHelper::HandleGet()
	{
		json::value result;
		Get().then([&result](const http_response& response)
			{
				if (response.status_code() == status_codes::OK)
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
