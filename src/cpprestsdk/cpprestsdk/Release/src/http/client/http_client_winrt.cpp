/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * HTTP Library: Client-side APIs.
 *
 * This file contains the implementation for the Windows Runtime, using XML HTTP Extended Request.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include "../common/internal_http_helpers.h"
#include "http_client_impl.h"
#include <Strsafe.h>
// Important for WP8
#if !defined(__WRL_NO_DEFAULT_LIB__)
#define __WRL_NO_DEFAULT_LIB__
#endif
#include <msxml6.h>
#include <wrl.h>
using namespace std;
using namespace Platform;
using namespace Microsoft::WRL;

namespace web
{
namespace http
{
namespace client
{
namespace details
{
// Additional information necessary to track a WinRT request.
class winrt_request_context final : public request_context
{
public:
    // Factory function to create requests on the heap.
    static std::shared_ptr<request_context> create_request_context(
        const std::shared_ptr<_http_client_communicator>& client, http_request& request)
    {
        return std::make_shared<winrt_request_context>(client, request);
    }

    Microsoft::WRL::ComPtr<IXMLHTTPRequest2> m_hRequest;
    std::exception_ptr m_exceptionPtr;

    // Request contexts must be created through factory function.
    // But constructor needs to be public for make_shared to access.
    winrt_request_context(const std::shared_ptr<_http_client_communicator>& client, http_request& request)
        : request_context(client, request), m_hRequest(nullptr), m_exceptionPtr()
    {
    }
};

// Implementation of IXMLHTTPRequest2Callback.
class HttpRequestCallback final : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IXMLHTTPRequest2Callback, FtmBase>
{
public:
    HttpRequestCallback(const std::shared_ptr<winrt_request_context>& request) : m_request(request) {}

    // Called when the HTTP request is being redirected to a new URL.
    HRESULT STDMETHODCALLTYPE OnRedirect(_In_opt_ IXMLHTTPRequest2*, __RPC__in_string const WCHAR*) { return S_OK; }

    // Called when HTTP headers have been received and processed.
    HRESULT STDMETHODCALLTYPE OnHeadersAvailable(_In_ IXMLHTTPRequest2* xmlReq,
                                                 DWORD dw,
                                                 __RPC__in_string const WCHAR* phrase)
    {
        http_response& response = m_request->m_response;
        response.set_status_code((http::status_code)dw);
        response.set_reason_phrase(phrase);

        utf16char* hdrStr = nullptr;
        HRESULT hr = xmlReq->GetAllResponseHeaders(&hdrStr);
        if (SUCCEEDED(hr))
        {
            try
            {
                auto progress = m_request->m_request._get_impl()->_progress_handler();
                if (progress && m_request->m_uploaded == 0)
                {
                    (*progress)(message_direction::upload, 0);
                }

                web::http::details::parse_headers_string(hdrStr, response.headers());
                m_request->complete_headers();
            }
            catch (...)
            {
                m_request->m_exceptionPtr = std::current_exception();
                hr = ERROR_UNHANDLED_EXCEPTION;
            }
        }

        if (hdrStr != nullptr)
        {
            ::CoTaskMemFree(hdrStr);
            hdrStr = nullptr;
        }

        return hr;
    }

    // Called when a portion of the entity body has been received.
    HRESULT STDMETHODCALLTYPE OnDataAvailable(_In_opt_ IXMLHTTPRequest2*, _In_opt_ ISequentialStream*) { return S_OK; }

    // Called when the entire entity response has been received.
    HRESULT STDMETHODCALLTYPE OnResponseReceived(_In_opt_ IXMLHTTPRequest2*, _In_opt_ ISequentialStream*)
    {
        auto progress = m_request->m_request._get_impl()->_progress_handler();
        if (progress && m_request->m_downloaded == 0)
        {
            try
            {
                (*progress)(message_direction::download, 0);
            }
            catch (...)
            {
                m_request->m_exceptionPtr = std::current_exception();
            }
        }

        if (m_request->m_exceptionPtr != nullptr)
            m_request->report_exception(m_request->m_exceptionPtr);
        else
            m_request->complete_request(m_request->m_downloaded);

        // Break the circular reference loop.
        //     - winrt_request_context holds a reference to IXmlHttpRequest2
        //     - IXmlHttpRequest2 holds a reference to HttpRequestCallback
        //     - HttpRequestCallback holds a reference to winrt_request_context
        //
        // Not releasing the winrt_request_context below previously worked due to the
        // implementation of IXmlHttpRequest2, after calling OnError/OnResponseReceived
        // it would immediately release its reference to HttpRequestCallback. However
        // it since has been discovered on Xbox that the implementation is different,
        // the reference to HttpRequestCallback is NOT immediately released and is only
        // done at destruction of IXmlHttpRequest2.
        //
        // To be safe we now will break the circular reference.
        m_request.reset();

        return S_OK;
    }

    // Called when an error occurs during the HTTP request.
    HRESULT STDMETHODCALLTYPE OnError(_In_opt_ IXMLHTTPRequest2*, HRESULT hrError)
    {
        if (m_request->m_exceptionPtr == nullptr)
        {
            std::wstring msg(L"IXMLHttpRequest2Callback::OnError: ");
            msg.append(std::to_wstring(hrError));
            msg.append(L": ");
            msg.append(utility::conversions::to_string_t(utility::details::windows_category().message(hrError)));
            m_request->report_error(hrError, msg);
        }
        else
        {
            m_request->report_exception(m_request->m_exceptionPtr);
        }

        // Break the circular reference loop.
        // See full explanation in OnResponseReceived
        m_request.reset();

        return S_OK;
    }

private:
    std::shared_ptr<winrt_request_context> m_request;
};

/// <summary>
/// This class acts as a bridge for the underlying request stream.
/// </summary>
/// <remarks>
/// These operations are completely synchronous, so it's important to block on both
/// read and write operations. The I/O will be done off the UI thread, so there is no risk
/// of causing the UI to become unresponsive.
/// </remarks>
class IRequestStream final
    : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<ClassicCom>, ISequentialStream>
{
public:
    IRequestStream(const std::weak_ptr<winrt_request_context>& context,
                   size_t read_length = (std::numeric_limits<size_t>::max)())
        : m_context(context), m_read_length(read_length)
    {
        // read_length is the initial length of the ISequentialStream that is available for read
        // This is required because IXHR2 attempts to read more data that what is specified by
        // the content_length. (Specifically, it appears to be reading 128K chunks regardless of
        // the content_length specified).
    }

    virtual HRESULT STDMETHODCALLTYPE Read(_Out_writes_(cb) void* pv, _In_ ULONG cb, _Out_ ULONG* pcbRead)
    {
        auto context = m_context.lock();
        if (context == nullptr)
        {
            // OnError has already been called so just error out
            return STG_E_READFAULT;
        }

        try
        {
            auto buffer = context->_get_readbuffer();

            // Do not read more than the specified read_length
            msl::safeint3::SafeInt<size_t> safe_count = static_cast<size_t>(cb);
            size_t size_to_read = safe_count.Min(m_read_length);

            const size_t count = buffer.getn((uint8_t*)pv, size_to_read).get();
            *pcbRead = (ULONG)count;
            if (count == 0 && size_to_read != 0)
            {
                return STG_E_READFAULT;
            }

            _ASSERTE(count != static_cast<size_t>(-1));
            _ASSERTE(m_read_length >= count);
            m_read_length -= count;

            auto progress = context->m_request._get_impl()->_progress_handler();
            if (progress && count > 0)
            {
                context->m_uploaded += count;
                try
                {
                    (*progress)(message_direction::upload, context->m_uploaded);
                }
                catch (...)
                {
                    context->m_exceptionPtr = std::current_exception();
                    return STG_E_READFAULT;
                }
            }

            return S_OK;
        }
        catch (...)
        {
            context->m_exceptionPtr = std::current_exception();
            return STG_E_READFAULT;
        }
    }

    virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv,
                                            _In_ ULONG cb,
                                            _Out_opt_ ULONG* pcbWritten)
    {
        (void)pv;
        (void)cb;
        (void)pcbWritten;
        return E_NOTIMPL;
    }

private:
    // The request context controls the lifetime of this class so we only hold a weak_ptr.
    std::weak_ptr<winrt_request_context> m_context;

    // Length of the ISequentialStream for reads. This is equivalent
    // to the amount of data that the ISequentialStream is allowed
    // to read from the underlying stream buffer.
    size_t m_read_length;
};

/// <summary>
/// This class acts as a bridge for the underlying response stream.
/// </summary>
/// <remarks>
/// These operations are completely synchronous, so it's important to block on both
/// read and write operations. The I/O will be done off the UI thread, so there is no risk
/// of causing the UI to become unresponsive.
/// </remarks>
class IResponseStream final
    : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<ClassicCom>, ISequentialStream>
{
public:
    IResponseStream(const std::weak_ptr<winrt_request_context>& context) : m_context(context) {}

    virtual HRESULT STDMETHODCALLTYPE Write(_In_reads_bytes_(cb) const void* pv,
                                            _In_ ULONG cb,
                                            _Out_opt_ ULONG* pcbWritten)
    {
        auto context = m_context.lock();
        if (context == nullptr)
        {
            // OnError has already been called so just error out
            return STG_E_CANTSAVE;
        }

        if (pcbWritten != nullptr)
        {
            *pcbWritten = 0;
        }

        if (cb == 0)
        {
            return S_OK;
        }

        try
        {
            auto buffer = context->_get_writebuffer();
            const size_t count =
                buffer.putn_nocopy(reinterpret_cast<const uint8_t*>(pv), static_cast<size_t>(cb)).get();

            _ASSERTE(count != static_cast<size_t>(-1));
            _ASSERTE(count <= static_cast<size_t>(ULONG_MAX));
            if (pcbWritten != nullptr)
            {
                *pcbWritten = (ULONG)count;
            }
            context->m_downloaded += count;

            auto progress = context->m_request._get_impl()->_progress_handler();
            if (progress && count > 0)
            {
                try
                {
                    (*progress)(message_direction::download, context->m_downloaded);
                }
                catch (...)
                {
                    context->m_exceptionPtr = std::current_exception();
                    return STG_E_CANTSAVE;
                }
            }

            return S_OK;
        }
        catch (...)
        {
            context->m_exceptionPtr = std::current_exception();
            return STG_E_CANTSAVE;
        }
    }

    virtual HRESULT STDMETHODCALLTYPE Read(_Out_writes_bytes_to_(cb, *pcbRead) void* pv,
                                           _In_ ULONG cb,
                                           _Out_ ULONG* pcbRead)
    {
        (void)pv;
        (void)cb;
        (void)pcbRead;
        return E_NOTIMPL;
    }

private:
    // The request context controls the lifetime of this class so we only hold a weak_ptr.
    std::weak_ptr<winrt_request_context> m_context;
};

// WinRT client.
class winrt_client final : public _http_client_communicator
{
public:
    winrt_client(http::uri&& address, http_client_config&& client_config)
        : _http_client_communicator(std::move(address), std::move(client_config))
    {
    }

    winrt_client(const winrt_client&) = delete;
    winrt_client& operator=(const winrt_client&) = delete;

    virtual pplx::task<http_response> propagate(http_request request) override
    {
        auto self = std::static_pointer_cast<_http_client_communicator>(shared_from_this());
        auto context = details::winrt_request_context::create_request_context(self, request);

        // Use a task to externally signal the final result and completion of the task.
        auto result_task = pplx::create_task(context->m_request_completion);

        // Asynchronously send the response with the HTTP client implementation.
        this->async_send_request(context);

        return result_task;
    }

protected:
    // Start sending request.
    virtual void send_request(_In_ const std::shared_ptr<request_context>& request) override
    {
        http_request& msg = request->m_request;
        auto winrt_context = std::static_pointer_cast<winrt_request_context>(request);

        if (!web::http::details::validate_method(msg.method()))
        {
            request->report_exception(http_exception(L"The method string is invalid."));
            return;
        }

        if (msg.method() == http::methods::TRCE)
        {
            // Not supported by WinInet. Generate a more specific exception than what WinInet does.
            request->report_exception(http_exception(L"TRACE is not supported"));
            return;
        }

        const size_t content_length = msg._get_impl()->_get_content_length();
        if (content_length == (std::numeric_limits<size_t>::max)())
        {
            // IXHR2 does not allow transfer encoding chunked. So the user is expected to set the content length
            request->report_exception(http_exception(L"Content length is not specified in the http headers"));
            return;
        }

        // Start sending HTTP request.
        HRESULT hr = CoCreateInstance(__uuidof(FreeThreadedXMLHTTP60),
                                      nullptr,
                                      CLSCTX_INPROC,
                                      __uuidof(IXMLHTTPRequest2),
                                      reinterpret_cast<void**>(winrt_context->m_hRequest.GetAddressOf()));
        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to create IXMLHTTPRequest2 instance");
            return;
        }

        utility::string_t encoded_resource = http::uri_builder(m_uri).append(msg.relative_uri()).to_string();

        const auto& config = client_config();
        const auto& client_cred = config.credentials();
        const auto& proxy = config.proxy();
        const auto& proxy_cred = proxy.credentials();
        if (!proxy.is_default())
        {
            request->report_exception(http_exception(L"Only a default proxy server is supported"));
            return;
        }

        // New scope to ensure plain text password is cleared as soon as possible.
        {
            utility::string_t username, proxy_username;
            const utility::char_t* password = nullptr;
            const utility::char_t* proxy_password = nullptr;
            ::web::details::plaintext_string password_plaintext, proxy_password_plaintext;

            if (client_cred.is_set())
            {
                username = client_cred.username();
                password_plaintext = client_cred._internal_decrypt();
                password = password_plaintext->c_str();
            }
            if (proxy_cred.is_set())
            {
                proxy_username = proxy_cred.username();
                proxy_password_plaintext = proxy_cred._internal_decrypt();
                proxy_password = proxy_password_plaintext->c_str();
            }

            hr = winrt_context->m_hRequest->Open(msg.method().c_str(),
                                                 encoded_resource.c_str(),
                                                 Make<HttpRequestCallback>(winrt_context).Get(),
                                                 username.c_str(),
                                                 password,
                                                 proxy_username.c_str(),
                                                 proxy_password);
        }
        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to open HTTP request");
            return;
        }

        // Suppress automatic prompts for user credentials, since they are already provided.
        hr = winrt_context->m_hRequest->SetProperty(XHR_PROP_NO_CRED_PROMPT, TRUE);
        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to set no credentials prompt property");
            return;
        }

        // Set timeout.
        ULONGLONG timeout = static_cast<ULONGLONG>(config.timeout<std::chrono::milliseconds>().count());
        timeout = (std::max<decltype(timeout)>)(timeout, (std::numeric_limits<decltype(timeout)>::min)() + 1);
        hr = winrt_context->m_hRequest->SetProperty(XHR_PROP_TIMEOUT, timeout);
        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to set HTTP request properties");
            return;
        }

        // If XHR_PROP_ONDATA_NEVER is defined in ixmlhttprequest.h or msxml6.h utilize it.
        // Specifies never to call OnDataAvailable improving performance and we
        // already don't use OnDataAvaliable anyway.
#ifdef XHR_PROP_ONDATA_NEVER
        hr = winrt_context->m_hRequest->SetProperty(XHR_PROP_ONDATA_THRESHOLD, XHR_PROP_ONDATA_NEVER);
        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to turn off on data threshold");
        }
#endif

        // Add headers.
        for (const auto& hdr : msg.headers())
        {
            winrt_context->m_hRequest->SetRequestHeader(hdr.first.c_str(), hdr.second.c_str());
        }

        // Set response stream.
        hr = winrt_context->m_hRequest->SetCustomResponseStream(Make<IResponseStream>(winrt_context).Get());
        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to set HTTP response stream");
            return;
        }

        // Call the callback function of user customized options
        try
        {
            config.invoke_nativehandle_options(winrt_context->m_hRequest.Get());
        }
        catch (...)
        {
            request->report_exception(std::current_exception());
            return;
        }

        if (content_length == 0)
        {
            hr = winrt_context->m_hRequest->Send(nullptr, 0);
        }
        else
        {
            if (msg.method() == http::methods::GET || msg.method() == http::methods::HEAD)
            {
                request->report_exception(http_exception(get_with_body_err_msg));
                return;
            }

            hr = winrt_context->m_hRequest->Send(Make<IRequestStream>(winrt_context, content_length).Get(),
                                                 content_length);
        }

        if (FAILED(hr))
        {
            request->report_error(hr, L"Failure to send HTTP request");
            return;
        }

        // Register for notification on cancellation to abort this request.
        if (msg._cancellation_token() != pplx::cancellation_token::none())
        {
            auto requestHandle = winrt_context->m_hRequest;

            // cancellation callback is unregistered when request is completed.
            winrt_context->m_cancellationRegistration =
                msg._cancellation_token().register_callback([requestHandle]() { requestHandle->Abort(); });
        }
    }
};

std::shared_ptr<_http_client_communicator> create_platform_final_pipeline_stage(uri&& base_uri,
                                                                                http_client_config&& client_config)
{
    return std::make_shared<details::winrt_client>(std::move(base_uri), std::move(client_config));
}

} // namespace details
} // namespace client
} // namespace http
} // namespace web
