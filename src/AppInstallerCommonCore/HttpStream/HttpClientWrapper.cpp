// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Public/AppInstallerStrings.h"
#include "HttpClientWrapper.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Security::Cryptography;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Web::Http;
using namespace winrt::Windows::Web::Http::Headers;
using namespace winrt::Windows::Web::Http::Filters;

// Note: this class is used by the HttpRandomAccessStream which is passed to the AppxPackaging COM API
// All exceptions thrown across dll boundaries should be WinRT exception not custom exceptions.
// The HRESULTs will be mapped to UI error code by the appropriate component
namespace AppInstaller::Utility::HttpStream
{
    std::future<std::shared_ptr<HttpClientWrapper>> HttpClientWrapper::CreateAsync(const Uri& uri)
    {
        std::shared_ptr<HttpClientWrapper> instance = std::make_shared<HttpClientWrapper>();

        // Use an HTTP filter to disable the default caching behavior and use the Most Recent caching behavior instead
        // so we don't use a stale cached resource. Note: this wrapper object is used in the custom HTTP stream implementation
        // so this affects the parsing of HTTP-based packages/bundles.
        HttpBaseProtocolFilter filter;
        filter.CacheControl().ReadBehavior(HttpCacheReadBehavior::MostRecent);
        instance->m_httpClient = HttpClient(filter);
        instance->m_requestUri = uri;

        instance->m_httpClient.DefaultRequestHeaders().Connection().Clear();
        instance->m_httpClient.DefaultRequestHeaders().Append(L"Connection", L"Keep-Alive");

        co_await instance->PopulateInfoAsync();

        co_return instance;
    }

    // this function will issue a HEAD request to determine the size of the file and the redirect URI
    std::future<void> HttpClientWrapper::PopulateInfoAsync()
    {
        HttpRequestMessage request(HttpMethod::Head(), m_requestUri);

        HttpResponseMessage response = co_await m_httpClient.SendRequestAsync(request, HttpCompletionOption::ResponseHeadersRead);

        THROW_HR_IF(
            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, response.StatusCode()),
            response.StatusCode() != HttpStatusCode::Ok);

        // Get the length from the response
        if (response.Content().Headers().HasKey(L"Content-Length"))
        {
            std::wstring contentLength(response.Content().Headers().Lookup(L"Content-Length"));
            m_sizeInBytes = std::stoll(contentLength);
        }
        else
        {
            m_sizeInBytes = 0;
        }

        // Get the extension from the redirect URI
        m_redirectUri = response.RequestMessage().RequestUri();

        m_contentType = response.Content().Headers().HasKey(L"Content-Type") ?
            response.Content().Headers().Lookup(L"Content-Type")
            : L"";

        // If the size wasn't resolved try with a GET 0-0 request
        if (m_sizeInBytes == 0)
        {
            co_await SendHttpRequestAsync(0, 1);
        }
    }

    std::future<IBuffer> HttpClientWrapper::SendHttpRequestAsync(
        _In_ ULONG64 startPosition,
        _In_ UINT32 requestedSizeInBytes)
    {
        unsigned long long endPosition = 0;

        winrt::check_hresult(ULong64Add(startPosition, requestedSizeInBytes, &endPosition));

        // Subtracting one should be safe, as the consumer of the stream should not request
        // an empty range, so this number can't go negative.
        endPosition -= 1;

        std::wstring rangeHeaderValue = L"bytes=" + std::to_wstring(startPosition) + L"-" + std::to_wstring(endPosition);

        HttpRequestMessage request(HttpMethod::Get(), m_requestUri);
        request.Headers().Append(L"Range", rangeHeaderValue);

        if (!Utility::IsEmptyOrWhitespace(m_etagHeader))
        {
            request.Headers().Append(L"If-Match", m_etagHeader);
        }

        if (!Utility::IsEmptyOrWhitespace(m_lastModifiedHeader))
        {
            request.Headers().Append(L"If-Unmodified-Since", m_lastModifiedHeader);
        }

        HttpResponseMessage response = co_await m_httpClient.SendRequestAsync(request, HttpCompletionOption::ResponseHeadersRead);
        HttpContentHeaderCollection contentHeaders = response.Content().Headers();

        if (response.StatusCode() != HttpStatusCode::PartialContent && startPosition != 0)
        {
            // throw HRESULT used for range-request error
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED));
        }

        if (response.Headers().HasKey(L"Accept-Ranges") &&
            Utility::ToLower(std::wstring(response.Headers().Lookup(L"Accept-Ranges"))) == L"none")
        {
            // throw HRESULT used for range-request error
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NO_RANGES_PROCESSED));
        }

        if (Utility::IsEmptyOrWhitespace(m_etagHeader) && response.Headers().HasKey(L"ETag"))
        {
            m_etagHeader = response.Headers().Lookup(L"ETag");
        }

        if (Utility::IsEmptyOrWhitespace(m_lastModifiedHeader) && contentHeaders.HasKey(L"Last-Modified"))
        {
            m_lastModifiedHeader = contentHeaders.Lookup(L"Last-Modified");
        }

        // If we don't know the size, parse it from the Content-Range field.
        if (m_sizeInBytes == 0 && contentHeaders.HasKey(L"Content-Range"))
        {
            // format: a-b/x where x is either a number or *
            std::wstring contentRange(contentHeaders.Lookup(L"Content-Range"));
            std::wstring length = contentRange.substr(contentRange.find(L"/") + 1);
            m_sizeInBytes = (length == L"*") ? 0 : std::stoll(length);
        }

        co_return co_await response.Content().ReadAsBufferAsync();
    }

    std::future<IBuffer> HttpClientWrapper::DownloadRangeAsync(
        const ULONG64 startPosition,
        const UINT32 requestedSizeInBytes,
        const InputStreamOptions& options)
    {
        std::vector<byte> byteArray(requestedSizeInBytes);
        IBuffer buffer = CryptographicBuffer::CreateFromByteArray(byteArray);

        co_return co_await SendHttpRequestAsync(startPosition, requestedSizeInBytes);
    }
}