// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <winget/HttpClientHelper.h>
#include <winget/NetworkSettings.h>
#include <winhttp.h>

namespace AppInstaller::Http
{
    namespace
    {
        // If the caller does not pass in a user agent header, put the default one on the request.
        void EnsureDefaultUserAgent(web::http::http_request& request)
        {
            static utility::string_t c_defaultUserAgent = Utility::ConvertToUTF16(AppInstaller::Runtime::GetDefaultUserAgent());

            if (!request.headers().has(web::http::header_names::user_agent))
            {
                request.headers().add(web::http::header_names::user_agent, c_defaultUserAgent);
            }
        }

        void NativeHandleServerCertificateValidation(web::http::client::native_handle handle, const Certificates::PinningConfiguration& pinningConfiguration, ThreadLocalStorage::ThreadGlobals* threadGlobals)
        {
            decltype(threadGlobals->SetForCurrentThread()) previousThreadGlobals;
            if (threadGlobals)
            {
                previousThreadGlobals = threadGlobals->SetForCurrentThread();
            }

            HINTERNET requestHandle = reinterpret_cast<HINTERNET>(handle);

            // Get certificate and pass along to pinning config
            wil::unique_cert_context certContext;
            DWORD bufferSize = sizeof(&certContext);
            THROW_IF_WIN32_BOOL_FALSE(WinHttpQueryOption(requestHandle, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &certContext, &bufferSize));

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_PINNED_CERTIFICATE_MISMATCH, !pinningConfiguration.Validate(certContext.get()));
        }

        std::chrono::seconds GetRetryAfter(const web::http::http_headers& headers)
        {
            auto retryAfterHeader = headers.find(web::http::header_names::retry_after);
            if (retryAfterHeader != headers.end())
            {
                return AppInstaller::Utility::GetRetryAfter(retryAfterHeader->second.c_str());
            }

            return 0s;
        }
    }

    HttpClientHelper::HttpClientHelper(std::shared_ptr<web::http::http_pipeline_stage> stage)
        : m_defaultRequestHandlerStage(std::move(stage))
    {
        const auto& proxyUri = Settings::Network().GetProxyUri();
        if (proxyUri)
        {
            AICLI_LOG(Repo, Info, << "Setting proxy for REST HTTP Client helper to " << proxyUri.value());
            m_clientConfig.set_proxy(web::web_proxy{ Utility::ConvertToUTF16(proxyUri.value()) });
        }
        else
        {
            AICLI_LOG(Repo, Info, << "REST HTTP Client helper does not use proxy");
        }
    }

    pplx::task<web::http::http_response> HttpClientHelper::Post(
        const utility::string_t& uri,
        const web::json::value& body,
        const HttpClientHelper::HttpRequestHeaders& headers,
        const HttpClientHelper::HttpRequestHeaders& authHeaders) const
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

        // Add auth headers after logging
        for (auto& pair : authHeaders)
        {
            request.headers().add(pair.first, pair.second);
        }

        return client.request(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandlePost(
        const utility::string_t& uri,
        const web::json::value& body,
        const HttpClientHelper::HttpRequestHeaders& headers,
        const HttpClientHelper::HttpRequestHeaders& authHeaders,
        const HttpResponseHandler& customHandler) const try
    {
        web::http::http_response httpResponse;
        Post(uri, body, headers, authHeaders).then([&httpResponse](const web::http::http_response& response)
            {
                httpResponse = response;
            }).wait();

        if (customHandler)
        {
            auto handlerResult = customHandler(httpResponse);
            if (!handlerResult.UseDefaultHandling)
            {
                return std::move(handlerResult.Result);
            }
        }

        return ValidateAndExtractResponse(httpResponse);
    }
    catch (web::http::http_exception& exception)
    {
        RethrowAsWilException(exception);
    }

    pplx::task<web::http::http_response> HttpClientHelper::Get(
        const utility::string_t& uri,
        const HttpClientHelper::HttpRequestHeaders& headers,
        const HttpClientHelper::HttpRequestHeaders& authHeaders) const
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

        // Add auth headers after logging
        for (auto& pair : authHeaders)
        {
            request.headers().add(pair.first, pair.second);
        }

        return client.request(request);
    }

    std::optional<web::json::value> HttpClientHelper::HandleGet(
        const utility::string_t& uri,
        const HttpClientHelper::HttpRequestHeaders& headers,
        const HttpClientHelper::HttpRequestHeaders& authHeaders,
        const HttpResponseHandler& customHandler) const try
    {
        web::http::http_response httpResponse;
        Get(uri, headers, authHeaders).then([&httpResponse](const web::http::http_response& response)
            {
                httpResponse = response;
            }).wait();

        if (customHandler)
        {
            auto handlerResult = customHandler(httpResponse);
            if (!handlerResult.UseDefaultHandling)
            {
                return std::move(handlerResult.Result);
            }
        }

        return ValidateAndExtractResponse(httpResponse);
    }
    catch (web::http::http_exception& exception)
    {
        RethrowAsWilException(exception);
    }

    void HttpClientHelper::SetPinningConfiguration(const Certificates::PinningConfiguration& configuration, std::shared_ptr<ThreadLocalStorage::ThreadGlobals> threadGlobals)
    {
        m_clientConfig.set_nativehandle_servercertificate_validation([pinConfig = configuration, globals = std::move(threadGlobals)](web::http::client::native_handle handle)
            {
                NativeHandleServerCertificateValidation(handle, pinConfig, globals.get());
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
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTAPI_ENDPOINT_NOT_FOUND);

        case web::http::status_codes::NoContent:
            result = {};
            break;

        case web::http::status_codes::BadRequest:
            THROW_HR(APPINSTALLER_CLI_ERROR_RESTAPI_INTERNAL_ERROR);

        case web::http::status_codes::TooManyRequests:
        case web::http::status_codes::ServiceUnavailable:
            THROW_EXCEPTION(AppInstaller::Utility::ServiceUnavailableException(GetRetryAfter(response.headers())));

        default:
            THROW_HR(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, response.status_code()));
        }

        return result;
    }

    std::optional<web::json::value> HttpClientHelper::ExtractJsonResponse(const web::http::http_response& response) const
    {
        utility::string_t contentType = response.headers().content_type();

        THROW_HR_IF(APPINSTALLER_CLI_ERROR_RESTAPI_UNSUPPORTED_MIME_TYPE,
            !contentType._Starts_with(web::http::details::mime_types::application_json));

        return response.extract_json().get();
    }

    [[noreturn]] void HttpClientHelper::RethrowAsWilException(const web::http::http_exception& exception)
    {
        // Some http_exceptions have no error code; default to REST internal error.
        HRESULT toThrow = APPINSTALLER_CLI_ERROR_RESTAPI_INTERNAL_ERROR;

        // 99% of the time this code comes from GetLastError.
        // In a few cases it will be 400; as in the HTTP status code.
        // Since that is the one case that http_client_winhttp.cpp uses, we map it specifically.
        // In the event that this makes no sense, ERROR_THREAD_MODE_ALREADY_BACKGROUND is Win32 error 400.
        int errorValue = exception.error_code().value();
        if (errorValue == web::http::status_codes::BadRequest)
        {
            toThrow = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, web::http::status_codes::BadRequest);
        }
        else if (errorValue)
        {
            toThrow = HRESULT_FROM_WIN32(errorValue);
        }

        THROW_HR_MSG(toThrow, "%hs", exception.what());
    }
}
