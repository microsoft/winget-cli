// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once


namespace AppInstaller::Utility::HttpStream
{
    // Wrapper around HTTP client. When created, an object of this class will send a HTTP 
    // head request to determine the size of the data source.
    class HttpClientWrapper
    {
    public:
        static std::future<std::shared_ptr<HttpClientWrapper>> CreateAsync(const winrt::Windows::Foundation::Uri& uri);

        std::future<winrt::Windows::Storage::Streams::IBuffer> DownloadRangeAsync(
            const ULONG64 startPosition,
            const UINT32 requestedSizeInBytes,
            const winrt::Windows::Storage::Streams::InputStreamOptions& options);

        unsigned long long GetFullFileSize()
        {
            return m_sizeInBytes;
        }

        winrt::Windows::Foundation::Uri GetRedirectUri()
        {
            return m_redirectUri;
        }

        std::wstring GetContentType()
        {
            return m_contentType;
        }

    private:
        winrt::Windows::Web::Http::HttpClient m_httpClient;
        winrt::Windows::Foundation::Uri m_requestUri = nullptr;
        winrt::Windows::Foundation::Uri m_redirectUri = nullptr;
        std::wstring m_contentType;
        unsigned long long m_sizeInBytes = 0;
        std::wstring m_etagHeader;
        std::wstring m_lastModifiedHeader;

        std::future<void> PopulateInfoAsync();

        std::future<winrt::Windows::Storage::Streams::IBuffer> SendHttpRequestAsync(
            _In_ ULONG64 startPosition,
            _In_ UINT32 requestedSizeInBytes);
    };
}