// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "HttpClientWrapper.h"
#include "HttpLocalCache.h"

namespace AppInstaller::Utility::HttpStream
{
    // Provides an implementation of a random access stream over HTTP that supports
    // range-based fetching. This is intended to be used by AppxPackageReader.
    //
    // Note: If the server doesn't support HTTP ranges, this implementation will throw an exception.
    class HttpRandomAccessStream : public winrt::implements<
        HttpRandomAccessStream,
        winrt::Windows::Storage::Streams::IRandomAccessStream,
        winrt::Windows::Storage::Streams::IInputStream>
    {
    public:
        static winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::Streams::IRandomAccessStream> CreateAsync(
            const winrt::Windows::Foundation::Uri& uri);
        uint64_t Size() const;
        void Size(uint64_t value);
        uint64_t Position() const;
        bool CanRead() const;
        bool CanWrite() const;
        winrt::Windows::Storage::Streams::IInputStream GetInputStreamAt(uint64_t position) const;
        winrt::Windows::Storage::Streams::IOutputStream GetOutputStreamAt(uint64_t position) const;
        winrt::Windows::Storage::Streams::IRandomAccessStream CloneStream() const;
        void Seek(uint64_t position);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Windows::Storage::Streams::IBuffer, uint32_t> ReadAsync(
            winrt::Windows::Storage::Streams::IBuffer buffer,
            uint32_t count,
            winrt::Windows::Storage::Streams::InputStreamOptions options);

    private:
        std::shared_ptr<HttpClientWrapper> m_httpHelper;
        std::unique_ptr<HttpLocalCache> m_httpLocalCache;
        unsigned long long m_size = 0;
        unsigned long long m_requestedPosition = 0;
    };
}