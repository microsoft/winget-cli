// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "pch.h"
#include "HttpClientWrapper.h"

namespace AppInstaller::Utility::HttpStream
{
    // Represents an entry in the cache.
    struct CachedPage
    {
        int lastAccessCounter;
        winrt::Windows::Storage::Streams::IBuffer buffer;
    };

    // A cache used internally by the custom HttpRandomAccessStream to reduce round-trips
    class HttpLocalCache
    {
    public:
        const UINT32 PAGE_SIZE = 2 << 16;   // each entry in the cache is 64 KB
        const UINT32 MAX_PAGES = 200;       // cache size capped at 12.5 MB (200 * 64KB)

        // Returns a buffer matching the requested range by reading the parts of the range that are cached
        // and downloading the rest using the provided httpClientWrapper object
        std::future<winrt::Windows::Storage::Streams::IBuffer> ReadFromCacheAndDownloadIfNecessaryAsync(
            const ULONG64 requestedPosition,
            const UINT32 requestedSize,
            HttpClientWrapper* httpClientWrapper,
            winrt::Windows::Storage::Streams::InputStreamOptions httpInputStreamOptions);

    private:
        std::map<ULONG64, CachedPage> m_localCache;
        UINT32 m_accessCounter = 0U;

        // Returns a vector of all pages corresponding to a range, and another (subset)
        // vector of the pages missing from the cache.
        void FindCachePages(
            const ULONG64 requestedPosition,
            const UINT32 requestedSize,
            std::vector<ULONG64>& allPages,
            std::vector<ULONG64>& unsatisfiablePages);

        void SaveBufferToCache(const winrt::Windows::Storage::Streams::IBuffer& buffer, const ULONG64 firstPageOffset);

        winrt::Windows::Storage::Streams::IBuffer ReadPageFromCache(const ULONG64 pageOffset);

        void VacateStaleEntriesFromCache();

        std::future<void> DownloadAndSaveToCacheAysnc(
            const std::vector<ULONG64> unsatisfiablePages,
            HttpClientWrapper* httpClientWrapper,
            const winrt::Windows::Storage::Streams::InputStreamOptions httpInputStreamOptions);

        winrt::Windows::Storage::Streams::IBuffer TrimBufferToSatisfyRequest(
            const winrt::Windows::Storage::Streams::IBuffer& constructedBuffer,
            const ULONG64 requestedPosition,
            const UINT32 requestedSize,
            const std::vector<ULONG64> allPages);

        winrt::Windows::Storage::Streams::IBuffer CreateTrimmedBuffer(
            const winrt::Windows::Storage::Streams::IBuffer& originalBuffer,
            UINT32 trimStartIndex,
            UINT32 size);

        winrt::Windows::Storage::Streams::IBuffer ConcatenateBuffers(
            const winrt::Windows::Storage::Streams::IBuffer& buffer1,
            const winrt::Windows::Storage::Streams::IBuffer& buffer2);
    };
}