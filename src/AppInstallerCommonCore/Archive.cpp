// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

namespace AppInstaller::Archive
{
    HRESULT ExtractArchive(const std::filesystem::path& archivePath, const std::filesystem::path& destPath)
    {
        IFileOperation* pFileOperation;
        HRESULT hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pFileOperation));

        if (SUCCEEDED(hr))
        {
            hr = pFileOperation->SetOperationFlags(FOF_NO_UI);
            if (SUCCEEDED(hr))
            {
                IShellItem* pShellItemTo;
                hr = SHCreateItemFromParsingName(destPath.c_str(), NULL, IID_PPV_ARGS(&pShellItemTo));
                if (SUCCEEDED(hr))
                {
                    PIDLIST_ABSOLUTE pidlParent;
                    IShellFolder* pArchiveShellFolder;
                    IEnumIDList* pEnumIdList;
                    hr = SHParseDisplayName(archivePath.c_str(), NULL, &pidlParent, 0, NULL);
                    if (SUCCEEDED(hr))
                    {
                        hr = SHBindToObject(NULL, pidlParent, NULL, IID_PPV_ARGS(&pArchiveShellFolder));
                        if (SUCCEEDED(hr))
                        {
                            hr = pArchiveShellFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnumIdList);
                            if (SUCCEEDED(hr))
                            {
                                LPITEMIDLIST pidlChild;
                                ULONG nFetched;
                                while (hr == S_OK && pEnumIdList->Next(1, &pidlChild, &nFetched) == S_OK && nFetched == 1)
                                {
                                    IShellItem* pShellItemFrom;
                                    STRRET strFolderName;
                                    WCHAR szFolderName[MAX_PATH];
                                    if ((pArchiveShellFolder->GetDisplayNameOf(pidlChild, SHGDN_INFOLDER, &strFolderName) == S_OK) &&
                                        (StrRetToBuf(&strFolderName, pidlChild, szFolderName, MAX_PATH) == S_OK))
                                    {
                                        hr = SHCreateItemWithParent(pidlParent, pArchiveShellFolder, pidlChild, IID_PPV_ARGS(&pShellItemFrom));
                                        if (SUCCEEDED(hr))
                                        {
                                            hr = pFileOperation->CopyItem(pShellItemFrom, pShellItemTo, NULL, NULL);
                                        }
                                        pShellItemFrom->Release();
                                    }
                                    ILFree(pidlChild);
                                }

                                hr = pFileOperation->PerformOperations();
                                pEnumIdList->Release();
                            }
                            pArchiveShellFolder->Release();
                        }
                        ILFree(pidlParent);
                    }
                    pShellItemTo->Release();
                }
            }
            pFileOperation->Release();
        }

        return hr;
    }
}