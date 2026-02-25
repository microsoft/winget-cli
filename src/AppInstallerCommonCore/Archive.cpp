// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Archive.h"

namespace AppInstaller::Archive
{
    using unique_pidlist_absolute = wil::unique_any<PIDLIST_ABSOLUTE, decltype(&::CoTaskMemFree), ::CoTaskMemFree>;
    using unique_lpitemidlist = wil::unique_any<LPITEMIDLIST, decltype(&::CoTaskMemFree), ::CoTaskMemFree>;

    HRESULT TryExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath)
    {
        wil::com_ptr<IFileOperation> pFileOperation;
        RETURN_IF_FAILED(CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOperation)));
        RETURN_IF_FAILED(pFileOperation->SetOperationFlags(FOF_NO_UI));

        wil::com_ptr<IShellItem> pShellItemTo;
        RETURN_IF_FAILED(SHCreateItemFromParsingName(destPath.c_str(), NULL, IID_PPV_ARGS(&pShellItemTo)));

        unique_pidlist_absolute pidlFull;
        RETURN_IF_FAILED(SHParseDisplayName(archivePath.c_str(), NULL, &pidlFull, 0, NULL));

        wil::com_ptr<IShellFolder> pArchiveShellFolder;
        RETURN_IF_FAILED(SHBindToObject(NULL, pidlFull.get(), NULL, IID_PPV_ARGS(&pArchiveShellFolder)));

        wil::com_ptr<IEnumIDList> pEnumIdList;
        RETURN_IF_FAILED(pArchiveShellFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIdList));

        unique_lpitemidlist pidlChild;
        ULONG nFetched;
        while (pEnumIdList->Next(1, wil::out_param_ptr<LPITEMIDLIST*>(pidlChild), &nFetched) == S_OK && nFetched == 1)
        {
            wil::com_ptr<IShellItem> pShellItemFrom;
            STRRET strFolderName;
            WCHAR szFolderName[MAX_PATH];
            RETURN_IF_FAILED(pArchiveShellFolder->GetDisplayNameOf(pidlChild.get(), SHGDN_INFOLDER | SHGDN_FORPARSING, &strFolderName));
            RETURN_IF_FAILED(StrRetToBuf(&strFolderName, pidlChild.get(), szFolderName, MAX_PATH));
            RETURN_IF_FAILED(SHCreateItemWithParent(pidlFull.get(), pArchiveShellFolder.get(), pidlChild.get(), IID_PPV_ARGS(&pShellItemFrom)));
            RETURN_IF_FAILED(pFileOperation->CopyItem(pShellItemFrom.get(), pShellItemTo.get(), NULL, NULL));
        }

        RETURN_IF_FAILED(pFileOperation->PerformOperations());
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

        HRESULT hr = S_OK;
        HAMSICONTEXT amsiContext;
        HAMSISESSION amsiSession;

        hr = AmsiInitialize(L"WinGet", &amsiContext);
        if (FAILED(hr))
        {
            return false;
        }

        hr = AmsiOpenSession(amsiContext, &amsiSession);
        if (FAILED(hr))
        {
            AmsiUninitialize(amsiContext);
            return false;
        }

        std::ifstream instream{ zipPath, std::ios::in | std::ios::binary };
        std::vector<uint8_t> data{ { std::istreambuf_iterator<char>{ instream } }, std::istreambuf_iterator<char>{} };

        AMSI_RESULT result = AMSI_RESULT_CLEAN;
        hr = AmsiScanBuffer(amsiContext, data.data(), static_cast<ULONG>(data.size()), zipPath.filename().c_str(), amsiSession, &result);

        AmsiCloseSession(amsiContext, amsiSession);
        AmsiUninitialize(amsiContext);

        return SUCCEEDED(hr) && (result == AMSI_RESULT_CLEAN || result == AMSI_RESULT_NOT_DETECTED);
    }
}

