// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "HttpLocalCache.h"

using namespace Windows::Storage::Streams;
using namespace winrt::Windows::Storage::Streams;
using namespace winrt::Windows::Security::Cryptography;

// Note: this class is used by the HttpRandomAccessStream which is passed to the AppxPackaging COM API
// All exceptions thrown across dll boundaries should be WinRT exception not custom exceptions.
// The HRESULTs will be mapped to UI error code by the appropriate component
namespace AppInstaller::Utility::HttpStream
{
    std::future<IBuffer> HttpLocalCache::ReadFromCacheAndDownloadIfNecessaryAsync(
        const ULONG64 requestedPosition,
        const UINT32 requestedSize,
        HttpClientWrapper* httpClientWrapper,
        InputStreamOptions httpInputStreamOptions)
    {
        // Increment cache access counter user for implementing LRU replacement
        m_accessCounter++;

        // Find all the pages for the given request, and the pages that are missing
        std::vector<ULONG64> allPages;
        std::vector<ULONG64> unsatisfiablePages;
        FindCachePages(requestedPosition, requestedSize, allPages, unsatisfiablePages);

        // download the missing pages
        co_await DownloadAndSaveToCacheAysnc(
            unsatisfiablePages,
            httpClientWrapper,
            httpInputStreamOptions);

        // At this point, everything should be in the cache
        IBuffer constructedBuffer = {};

        for (UINT32 i = 0; i < allPages.size(); i++)
        {
            UINT64 pageOffset = allPages[i];
            IBuffer cachedPageBuffer = ReadPageFromCache(pageOffset);
            constructedBuffer = ConcatenateBuffers(constructedBuffer, cachedPageBuffer);
        }

        // trim buffer to match requested range
        IBuffer requestedBuffer = TrimBufferToSatisfyRequest(
            constructedBuffer,
            requestedPosition,
            requestedSize,
            allPages);

        VacateStaleEntriesFromCache();

        co_return requestedBuffer;
    }

    void HttpLocalCache::FindCachePages(
        ULONG64 requestedPosition,
        UINT32 requestedSize,
        std::vector<ULONG64>& allPages,
        std::vector<ULONG64>& unsatisfiablePages)
    {
        ULONG64 requestedEndPosition;
        ULONG64 currentPageOffset;
        winrt::check_hresult(ULong64Add(requestedPosition, requestedSize, &requestedEndPosition));
        winrt::check_hresult(ULong64Mult((requestedPosition / PAGE_SIZE), PAGE_SIZE, &currentPageOffset));

        // There's always at least one page for the range
        do
        {
            allPages.push_back(currentPageOffset);

            if (m_localCache.find(currentPageOffset) == m_localCache.end())
            {
                unsatisfiablePages.push_back(currentPageOffset);
            }

            winrt::check_hresult(ULong64Add(currentPageOffset, PAGE_SIZE, &currentPageOffset));

        } while (currentPageOffset < requestedEndPosition);
    }

    // Breaks the provided buffer into smaller buffers and saves them to the cache at the corresponding 
    // page offset position, starting at firstPageOffset. The smaller buffers are all PAGE_SIZE bytes,
    // except for the one corresponding to the last page in the file
    void HttpLocalCache::SaveBufferToCache(const IBuffer& buffer, const ULONG64 firstPageOffset)
    {
        UINT32 remainingBufferSize = buffer.Length();
        UINT32 currentBufferIndex = 0;
        ULONG64 currentPageOffset = firstPageOffset;

        while (remainingBufferSize > 0)
        {
            // Extract the sub-buffer
            UINT32 currentPageSize = std::min(remainingBufferSize, PAGE_SIZE);
            IBuffer currentPageBuffer = CreateTrimmedBuffer(buffer, currentBufferIndex, currentPageSize);

            // Add it to the cache
            CachedPage currentPage;
            currentPage.lastAccessCounter = m_accessCounter;
            currentPage.buffer = currentPageBuffer;
            m_localCache[currentPageOffset] = currentPage;

            // update loop vars
            winrt::check_hresult(UInt32Sub(remainingBufferSize, currentPageSize, &remainingBufferSize));
            winrt::check_hresult(UInt32Add(currentBufferIndex, currentPageSize, &currentBufferIndex));
            winrt::check_hresult(ULong64Add(currentPageOffset, PAGE_SIZE, &currentPageOffset));
        }
    }

    IBuffer HttpLocalCache::ReadPageFromCache(const ULONG64 pageOffset)
    {
        if (!(m_localCache.find(pageOffset) != m_localCache.end()))
        {
            THROW_HR(E_INVALIDARG);
        }

        CachedPage& page = m_localCache[pageOffset];
        page.lastAccessCounter = m_accessCounter;

        return page.buffer;
    }

    // Trims a buffer that was constructed (by fetching pages from cache and downloading missing pages)
    // in order to satisfy a request and return the exact buffer the consumer asked for.
    IBuffer HttpLocalCache::TrimBufferToSatisfyRequest(
        const IBuffer& constructedBuffer,
        const ULONG64 requestedPosition,
        const UINT32 requestedSize,
        const std::vector<ULONG64> allPages)
    {
        ULONG64 fullBufferStartOffset = allPages[0];

        ULONG64 trimmedBufferStartRelativeIndex;
        winrt::check_hresult(ULong64Sub(requestedPosition, fullBufferStartOffset, &trimmedBufferStartRelativeIndex));

        IBuffer requestedBuffer = CreateTrimmedBuffer(
            constructedBuffer,
            (UINT32)trimmedBufferStartRelativeIndex, // Conversion is safe as buffer size is a UINT32.
            requestedSize);

        return requestedBuffer;
    }

    // Downloads a chunk of the file, saves it to the cache, and returns the corresponding buffer
    // If the requested size is 0, this method returns an empty buffer without making HTTP calls
    std::future<void> HttpLocalCache::DownloadAndSaveToCacheAysnc(
        const std::vector<ULONG64> unsatisfiablePages,
        HttpClientWrapper* httpClientWrapper,
        InputStreamOptions httpInputStreamOptions)
    {
        // Determine the download job
        // To make things easy, we will download the contiguous range that includes all the unsatisfiable ranges.
        // Note that in theory, this may include cached pages. However, this situation is rarely expected to happen,
        // if at all. The package reader usually reads things in chunks of 64 KB or less, so, we should expect to 
        // always have up to two satisfiable and unsatisfiable pages in total.
        UINT64 fileSize = httpClientWrapper->GetFullFileSize();
        ULONG64 downloadJobStartPosition = 0U;
        ULONG64 downloadJobEndPosition = 0U;
        ULONG64 downloadJobSize = 0U;
        if (unsatisfiablePages.size() > 0U)
        {
            downloadJobStartPosition = unsatisfiablePages[0];
            ULONG64 lastUnsatisfiableJob = unsatisfiablePages[unsatisfiablePages.size() - 1];
            winrt::check_hresult(ULong64Add(lastUnsatisfiableJob, PAGE_SIZE, &downloadJobEndPosition));

            // make sure to not overflow file size
            downloadJobEndPosition = std::min(downloadJobEndPosition, fileSize);
            winrt::check_hresult(ULong64Sub(downloadJobEndPosition, downloadJobStartPosition, &downloadJobSize));
        }

        if (downloadJobSize != 0U)
        {
            // start download job
            IBuffer downloadedBuffer = co_await httpClientWrapper->DownloadRangeAsync(
                downloadJobStartPosition,
                (UINT32)downloadJobSize,
                httpInputStreamOptions);

            SaveBufferToCache(downloadedBuffer, downloadJobStartPosition);
        }
    }

    void HttpLocalCache::VacateStaleEntriesFromCache()
    {
        // Copy page offsets into vector and sort by the access counter
        std::vector<std::pair<UINT64, int>> orderedPageOffsets;
        for (auto pageIter = m_localCache.begin(); pageIter != m_localCache.end(); pageIter++)
        {
            orderedPageOffsets.push_back(std::pair<UINT64, int>(pageIter->first, pageIter->second.lastAccessCounter));
        }

        // Compare function to sort by access counter
        auto cmp = [](std::pair<UINT64, int> const & a, std::pair<UINT64, int> const & b)
        {
            return a.second != b.second ? a.second < b.second : a.first < b.first;
        };

        std::sort(orderedPageOffsets.begin(), orderedPageOffsets.end(), cmp);

        for (auto pageIter = orderedPageOffsets.begin(); pageIter != orderedPageOffsets.end(); pageIter++)
        {
            if (m_localCache.size() > MAX_PAGES)
            {
                m_localCache.erase(pageIter->first);
            }
            else
            {
                break;
            }
        }
    }

    IBuffer HttpLocalCache::CreateTrimmedBuffer(
        const IBuffer& originalBuffer,
        UINT32 trimStartIndex,
        UINT32 size)
    {
        originalBuffer.as<::IInspectable>();

        // Get the byte array from the IBuffer object
        Microsoft::WRL::ComPtr<IBufferByteAccess> bufferByteAccess;
        ::IInspectable* bufferAbi = (::IInspectable*)winrt::get_abi(originalBuffer);
        bufferAbi->QueryInterface(IID_PPV_ARGS(&bufferByteAccess));
        byte* byteBuffer = nullptr;
        bufferByteAccess->Buffer(&byteBuffer);

        // Create the array of bytes holding the trimmed bytes
        IBuffer trimmedBuffer = CryptographicBuffer::CreateFromByteArray(
            { byteBuffer + trimStartIndex, byteBuffer + trimStartIndex + size });

        return trimmedBuffer;
    }

    IBuffer HttpLocalCache::ConcatenateBuffers(const IBuffer& buffer1, const IBuffer& buffer2)
    {
        DataWriter writer;
        writer.WriteBuffer(buffer1);
        writer.WriteBuffer(buffer2);
        return writer.DetachBuffer();
    }
}