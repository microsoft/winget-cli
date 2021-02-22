// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "cpprest/http_client.h"
#include "cpprest/json.h"

using namespace web;
using namespace web::json;
using namespace web::http;
using namespace web::http::client;

namespace AppInstaller::Repository::Rest
{
	struct HttpClientHelper
	{
		HttpClientHelper(utility::string_t url);

		pplx::task<web::http::http_response> Post(const json::value& body);

		json::value HandlePost(const json::value& body);

		pplx::task<web::http::http_response> Get();

		json::value HandleGet();

	protected:
		pplx::task<web::http::http_response> MakeRequest(web::http::http_request req);

	private:
		http_client m_client;
		utility::string_t m_url;
	};
}
