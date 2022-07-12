// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

namespace AppInstaller::Archive
{
    HRESULT TryExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath)
    {
        wil::com_ptr<IFileOperation> pFileOperation;
        RETURN_IF_FAILED(CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOperation)));
        RETURN_IF_FAILED(pFileOperation->SetOperationFlags(FOF_NO_UI));

        wil::com_ptr<IShellItem> pShellItemTo;
        RETURN_IF_FAILED(SHCreateItemFromParsingName(destPath.c_str(), NULL, IID_PPV_ARGS(&pShellItemTo)));

        wil::unique_cotaskmem_ptr<PIDLIST_ABSOLUTE> pidlParent;
        wil::com_ptr<IEnumIDList> pEnumIdList;
        RETURN_IF_FAILED(SHParseDisplayName(archivePath.c_str(), NULL, pidlParent.get(), 0, NULL));
        wil::com_ptr<IShellFolder> pArchiveShellFolder;
        RETURN_IF_FAILED(SHBindToObject(NULL, *pidlParent, NULL, IID_PPV_ARGS(&pArchiveShellFolder)));
        RETURN_IF_FAILED(pArchiveShellFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIdList));

        wil::unique_cotaskmem_ptr<LPITEMIDLIST> pidlChild;
        ULONG nFetched;
        while (pEnumIdList->Next(1, pidlChild.get(), &nFetched) == S_OK && nFetched == 1)
        {
            wil::com_ptr<IShellItem> pShellItemFrom;
            STRRET strFolderName;
            WCHAR szFolderName[MAX_PATH];
            if ((pArchiveShellFolder->GetDisplayNameOf(*pidlChild, SHGDN_INFOLDER, &strFolderName) == S_OK) &&
                (StrRetToBuf(&strFolderName, *pidlChild, szFolderName, MAX_PATH) == S_OK))
            {
                RETURN_IF_FAILED(SHCreateItemWithParent(*pidlParent, pArchiveShellFolder.get(), *pidlChild, IID_PPV_ARGS(&pShellItemFrom)));
                RETURN_IF_FAILED(pFileOperation->CopyItem(pShellItemFrom.get(), pShellItemTo.get(), NULL, NULL));
            }
        }

        RETURN_IF_FAILED(pFileOperation->PerformOperations());
        return S_OK;
    }
}