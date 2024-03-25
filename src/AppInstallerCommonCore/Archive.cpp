// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Archive.h"
#include <bitfileextractor.hpp>

// TODO: Move include statement to pch.h and resolve build errors
#pragma warning( push )
#pragma warning ( disable : 4189 4244 26451 )
#include <pure.h>
#pragma warning ( pop )

namespace AppInstaller::Archive
{
    using unique_pidlist_absolute = wil::unique_any<PIDLIST_ABSOLUTE, decltype(&::CoTaskMemFree), ::CoTaskMemFree>;
    using unique_lpitemidlist = wil::unique_any<LPITEMIDLIST, decltype(&::CoTaskMemFree), ::CoTaskMemFree>;
    

    HRESULT TryExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath)
    {
        try {
            using namespace bit7z;
            Bit7zLibrary lib{ "7za.dll" };
            BitFileExtractor extractor{ lib, BitFormat::SevenZip };
            extractor.extract(archivePath.string(), destPath.string());
        }catch (const bit7z::BitException& ex) {
            RETURN_IF_FAILED(ex.hresultCode());
        }
        return S_OK;
        
    } 


#ifndef AICLI_DISABLE_TEST_HOOKS
    static bool* s_ScanArchiveResult_TestHook_Override = nullptr;

    void TestHook_SetScanArchiveResult_Override(bool* status)
    {
        s_ScanArchiveResult_TestHook_Override = status;
    }
#endif

    bool ScanZipFile(const std::filesystem::path& zipPath)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_ScanArchiveResult_TestHook_Override)
        {
            return *s_ScanArchiveResult_TestHook_Override;
        }
#endif

        std::ifstream instream{ zipPath, std::ios::in | std::ios::binary };
        std::vector<uint8_t> data{ { std::istreambuf_iterator<char>{ instream } }, std::istreambuf_iterator<char>{} };

        uint8_t* buffer = &data[0];
        uint64_t flag = 0;
        int scanResult = pure_zip(buffer, data.size(), flag);

        return scanResult == 0;
    }
}
