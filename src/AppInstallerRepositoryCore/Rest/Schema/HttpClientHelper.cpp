// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "HttpClientHelper.h"

namespace AppInstaller::Repository::Rest::Schema
{
    namespace
    {
        // If the caller does not pass in a user agent header, put the default one on the request.
        void EnsureDefaultUserAgent(web::http::http_request& request)
        {
            static utility::string_t c_defaultUserAgent = Utility::ConvertToUTF16(Runtime::GetDefaultUserAgent());

            if (!request.headers().has(web::http::header_names::user_agent))
            {
                request.headers().add(web::http::header_names::user_agent, c_defaultUserAgent);
            }
        }

        void NativeHandleServerCertificateValidation(web::http::client::native_handle handle, const Certificates::PinningConfiguration& pinningConfiguration)
        {
            HINTERNET requestHandle = reinterpret_cast<HINTERNET>(handle);

            // Get certificate and pass along to pinning config
            wil::unique_cert_context certContext;
            DWORD bufferSize = sizeof(&certContext);
            THROW_IF_WIN32_BOOL_FALSE(WinHttpQueryOption(requestHandle, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &certContext, &bufferSize));

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH, !pinningConfiguration.Validate(certContext.get()));
        }
    }

    HttpClientHelper::HttpClientHelper(std::shared_ptr<web::http::http_pipeline_stage> stage) : m_defaultRequestHandlerStage(std::move(stage)) {}

    pplx::task<web::http::http_response> HttpClientHelper::Post(
        const utility::string_t& uri, const web::json::value& body, const std::unordered_map<utility::string_t, utility::string_t>& headers) const
    {
        AICLI_LOG(Repo, Info, << "Sending http POST request to: " << utility::conversions::to_utf8string(uri));
        web::http::client::http_client client = GetClient(uri);
        web::http::http_request request{ web::http::methods::POST };
        request.headers().set_content_type(web::http::details::mime_types::application_json);
        request.set_body(body.serialize());

        // Add headers
        for (auto& pair : headers)
        {
            request.headers().add(pair.first, pair.second);
        }
        EnsureDefaultUserAgent(request);

        AICLI_LOG(Repo, Verbose, << "Http POST request details:\n" << utility::conversions::to_utf8string(request.to_string()));

        return client.request(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandlePost(
        const utility::string_t& uri, const web::json::value& body, const std::unordered_map<utility::string_t, utility::string_t>& headers) const
    {
        web::http::http_response httpResponse;
        HttpClientHelper::Post(uri, body, headers).then([&httpResponse](const web::http::http_response& response)
            {
                httpResponse = response;
            }).wait();

        return ValidateAndExtractResponse(httpResponse);
    }

    pplx::task<web::http::http_response> HttpClientHelper::Get(
        const utility::string_t& uri, const std::unordered_map<utility::string_t, utility::string_t>& headers) const
    {
        AICLI_LOG(Repo, Info, << "Sending http GET request to: " << utility::conversions::to_utf8string(uri));
        web::http::client::http_client client = GetClient(uri);
        web::http::http_request request{ web::http::methods::GET };
        request.headers().set_content_type(web::http::details::mime_types::application_json);

        // Add headers
        for (auto& pair : headers)
        {
            request.headers().add(pair.first, pair.second);
        }
        EnsureDefaultUserAgent(request);

        AICLI_LOG(Repo, Verbose, << "Http GET request details:\n" << utility::conversions::to_utf8string(request.to_string()));

        return client.request(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandleGet(
        const utility::string_t& uri, const std::unordered_map<utility::string_t, utility::string_t>& headers) const
    {
        web::http::http_response httpResponse;
        Get(uri, headers).then([&httpResponse](const web::http::http_response& response)
            {
                httpResponse = response;
            }).wait();

        return ValidateAndExtractResponse(httpResponse);
    }

    void HttpClientHelper::SetPinningConfiguration(const Certificates::PinningConfiguration& configuration)
    {
        m_clientConfig.set_nativehandle_servercertificate_validation([pinConfig = configuration](web::http::client::native_handle handle)
            {
                NativeHandleServerCertificateValidation(handle, pinConfig);
            });
    }

    web::http::client::http_client HttpClientHelper::GetClient(const utility::string_t& uri) const
    {
        web::http::client::http_client client{ uri, m_clientConfig };

        // Add default custom handlers if any.
        if (m_defaultRequestHandlerStage)
        {
            client.add_handler(m_defaultRequestHandlerStage);
        }

        return client;
    }

    std::optional<web::json::value> HttpClientHelper::ValidateAndExtractResponse(const web::http::http_response& response) const
    {
        AICLI_LOG(Repo, Info, << "Response status: " << response.status_code());
        // Ensure that we wait for the content to be ready before we log it; otherwise it will be truncated.
        AICLI_LOG_LARGE_STRING(Repo, Verbose, << "Response details:",
            response.content_ready().then([&](const web::http::http_response&) { return utility::conversions::to_utf8string(response.to_string()); }).get());

        std::optional<web::json::value> result;
        switch (response.status_code())
        {
        case web::http::status_codes::OK:
            result = ExtractJsonResponse(response);
            break;

        case web::http::status_codes::NotFound:
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTSOURCE_ENDPOINT_NOT_FOUND);
            break;

        case web::http::status_codes::NoContent:
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
