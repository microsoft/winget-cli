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

using web::http::http_request;

namespace AppInstaller::Repository::Rest
{
	struct HttpClientHelper
	{
		HttpClientHelper(utility::string_t url);

		pplx::task<web::http::http_response> Post(const json::value& body);

		json::value Handle_Post(const json::value& body);

		pplx::task<web::http::http_response> Get();

		json::value Handle_Get();

	protected:
		pplx::task<web::http::http_response> Make_Request(web::http::http_request req);

	private:
		http_client m_client;
		utility::string_t m_url;
	};
}