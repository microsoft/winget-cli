// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>
#include <compressapi.h>
#include <vector>
#include <string_view>

namespace AppInstaller::Compression
{
    // Contains a compressor from the Windows Compression API.
    struct Compressor
    {
        // Create a compressor using the given algorithm (see COMPRESS_ALGORITHM_*)
        Compressor(DWORD algorithm);

        // Compresses the given data.
        std::vector<uint8_t> Compress(std::string_view data);

        // Resets the compressor.
        void Reset();

        // Sets compressor information values.
        void SetInformation(COMPRESS_INFORMATION_CLASS information, DWORD value);

        // Gets compressor information values.
        DWORD GetInformation(COMPRESS_INFORMATION_CLASS information);

    private:
        wil::unique_any<COMPRESSOR_HANDLE, decltype(CloseCompressor), CloseCompressor> m_compressor;
    };

    // Contains a decompressor from the Windows Compression API.
    struct Decompressor
    {
        // Create a decompressor using the given algorithm (see COMPRESS_ALGORITHM_*)
        Decompressor(DWORD algorithm);

        // Decompresses the given data.
        std::vector<uint8_t> Decompress(const std::vector<uint8_t>& data);

        // Resets the decompressor.
        void Reset();

        // Sets decompressor information values.
        void SetInformation(COMPRESS_INFORMATION_CLASS information, DWORD value);

        // Gets decompressor information values.
        DWORD GetInformation(COMPRESS_INFORMATION_CLASS information);

    private:
        wil::unique_any<DECOMPRESSOR_HANDLE, decltype(CloseDecompressor), CloseDecompressor> m_decompressor;
    };
}
