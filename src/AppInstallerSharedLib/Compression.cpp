// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Compression.h"

namespace AppInstaller::Compression
{
    Compressor::Compressor(DWORD algorithm)
    {
        THROW_IF_WIN32_BOOL_FALSE(CreateCompressor(algorithm, nullptr, &m_compressor));
    }

    std::vector<uint8_t> Compressor::Compress(std::string_view data)
    {
        std::vector<uint8_t> result;

        if (!data.empty())
        {
            SIZE_T compressedBufferSize = 0;
            THROW_HR_IF(E_UNEXPECTED, ::Compress(m_compressor.get(), data.data(), data.size(), nullptr, 0, &compressedBufferSize));
            THROW_LAST_ERROR_IF(GetLastError() != ERROR_INSUFFICIENT_BUFFER);

            result.resize(compressedBufferSize);

            SIZE_T compressedDataSize = 0;
            THROW_IF_WIN32_BOOL_FALSE(::Compress(m_compressor.get(), data.data(), data.size(), &result[0], result.size(), &compressedDataSize));

            result.resize(compressedDataSize);
        }

        return result;
    }

    void Compressor::Reset()
    {
        THROW_IF_WIN32_BOOL_FALSE(ResetCompressor(m_compressor.get()));
    }

    void Compressor::SetInformation(COMPRESS_INFORMATION_CLASS information, DWORD value)
    {
        THROW_IF_WIN32_BOOL_FALSE(SetCompressorInformation(m_compressor.get(), information, &value, sizeof(value)));
    }

    DWORD Compressor::GetInformation(COMPRESS_INFORMATION_CLASS information)
    {
        DWORD result = 0;
        THROW_IF_WIN32_BOOL_FALSE(QueryCompressorInformation(m_compressor.get(), information, &result, sizeof(result)));
        return result;
    }

    Decompressor::Decompressor(DWORD algorithm)
    {
        THROW_IF_WIN32_BOOL_FALSE(CreateDecompressor(algorithm, nullptr, &m_decompressor));
    }

    std::vector<uint8_t> Decompressor::Decompress(const std::vector<uint8_t>& data)
    {
        std::vector<uint8_t> result;

        if (!data.empty())
        {
            SIZE_T decompressedBufferSize = 0;
            THROW_HR_IF(E_UNEXPECTED, ::Decompress(m_decompressor.get(), data.data(), data.size(), nullptr, 0, &decompressedBufferSize));
            THROW_LAST_ERROR_IF(GetLastError() != ERROR_INSUFFICIENT_BUFFER);

            result.resize(decompressedBufferSize);

            SIZE_T decompressedDataSize = 0;
            THROW_IF_WIN32_BOOL_FALSE(::Decompress(m_decompressor.get(), data.data(), data.size(), &result[0], result.size(), &decompressedDataSize));

            result.resize(decompressedDataSize);
        }

        return result;
    }

    void Decompressor::Reset()
    {
        THROW_IF_WIN32_BOOL_FALSE(ResetDecompressor(m_decompressor.get()));
    }

    void Decompressor::SetInformation(COMPRESS_INFORMATION_CLASS information, DWORD value)
    {
        THROW_IF_WIN32_BOOL_FALSE(SetDecompressorInformation(m_decompressor.get(), information, &value, sizeof(value)));
    }

    DWORD Decompressor::GetInformation(COMPRESS_INFORMATION_CLASS information)
    {
        DWORD result = 0;
        THROW_IF_WIN32_BOOL_FALSE(QueryDecompressorInformation(m_decompressor.get(), information, &result, sizeof(result)));
        return result;
    }
}
