// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

using unique_pidlist_absolute = wil::unique_any<PIDLIST_ABSOLUTE, decltype(&::CoTaskMemFree), ::CoTaskMemFree>;
using unique_lpitemidlist = wil::unique_any<LPITEMIDLIST, decltype(&::CoTaskMemFree), ::CoTaskMemFree>;

namespace AppInstaller::Archive
{
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
            if ((pArchiveShellFolder->GetDisplayNameOf(pidlChild.get(), SHGDN_INFOLDER, &strFolderName) == S_OK) &&
                (StrRetToBuf(&strFolderName, pidlChild.get(), szFolderName, MAX_PATH) == S_OK))
            {
                RETURN_IF_FAILED(SHCreateItemWithParent(pidlFull.get(), pArchiveShellFolder.get(), pidlChild.get(), IID_PPV_ARGS(&pShellItemFrom)));
                RETURN_IF_FAILED(pFileOperation->CopyItem(pShellItemFrom.get(), pShellItemTo.get(), NULL, NULL));
            }
        }

        RETURN_IF_FAILED(pFileOperation->PerformOperations());
        return S_OK;
    }
}