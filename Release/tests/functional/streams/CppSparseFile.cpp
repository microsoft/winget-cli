/****************************** Module Header ******************************\
* Module Name:  CppSparseFile.cpp
* Project:      CppSparseFile
* URL: http://code.msdn.microsoft.com/windowsapps/CppSparseFile-7f28156b
* Copyright (c) Microsoft Corporation.
*
* CppSparseFile demonstrates the common operations on sparse files. A sparse
* file is a type of computer file that attempts to use file system space more
* efficiently when blocks allocated to the file are mostly empty. This is
* achieved by writing brief information (metadata) representing the empty
* blocks to disk instead of the actual "empty" space which makes up the
* block, using less disk space. You can find in this example the creation of
* sparse file, the detection of sparse attribute, the retrieval of sparse
* file size, and the query of sparse file layout.
*
* This source is subject to the Microsoft Public License.
* See http://www.microsoft.com/opensource/licenses.mspx#Ms-PL.
* All other rights reserved.
*
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
* EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
\***************************************************************************/

#pragma region Includes
#include "stdafx.h"

#include "CppSparseFile.h"
#pragma endregion

/*!
 * VolumeSupportsSparseFiles determines if the volume supports sparse streams.
 *
 * \param lpRootPathName
 * Volume root path e.g. C:\
 */
BOOL VolumeSupportsSparseFiles(LPCTSTR lpRootPathName)
{
    DWORD dwVolFlags;
    GetVolumeInformation(lpRootPathName, NULL, MAX_PATH, NULL, NULL, &dwVolFlags, NULL, MAX_PATH);

    return (dwVolFlags & FILE_SUPPORTS_SPARSE_FILES) ? TRUE : FALSE;
}

/*!
 * IsSparseFile determines if a file is sparse.
 *
 * \param lpFileName
 * File name
 */
BOOL IsSparseFile(LPCTSTR lpFileName)
{
    // Open the file for read
    HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    // Get file information
    BY_HANDLE_FILE_INFORMATION bhfi;
    GetFileInformationByHandle(hFile, &bhfi);
    CloseHandle(hFile);

    return (bhfi.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) ? TRUE : FALSE;
}

/*!
 * Get sparse file sizes.
 *
 * \param lpFileName
 * File name
 *
 * \see
 * http://msdn.microsoft.com/en-us/library/aa365276.aspx
 */
BOOL GetSparseFileSize(LPCTSTR lpFileName)
{
    // Retrieves the size of the specified file, in bytes. The size includes
    // both allocated ranges and sparse ranges.
    HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;
    LARGE_INTEGER liSparseFileSize;
    GetFileSizeEx(hFile, &liSparseFileSize);

    // Retrieves the file's actual size on disk, in bytes. The size does not
    // include the sparse ranges.
    LARGE_INTEGER liSparseFileCompressedSize;
    liSparseFileCompressedSize.LowPart =
        GetCompressedFileSize(lpFileName, (LPDWORD)&liSparseFileCompressedSize.HighPart);

    // Print the result
    wprintf(L"\nFile total size: %I64uKB\nActual size on disk: %I64uKB\n",
            liSparseFileSize.QuadPart / 1024,
            liSparseFileCompressedSize.QuadPart / 1024);

    CloseHandle(hFile);
    return TRUE;
}

/*!
 * Create a sparse file.
 *
 * \param lpFileName
 * The name of the sparse file
 */
HANDLE CreateSparseFile(LPCTSTR lpFileName)
{
    // Create a normal file
    HANDLE hSparseFile = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hSparseFile == INVALID_HANDLE_VALUE) return hSparseFile;

    // Use the DeviceIoControl function with the FSCTL_SET_SPARSE control
    // code to mark the file as sparse. If you don't mark the file as sparse,
    // the FSCTL_SET_ZERO_DATA control code will actually write zero bytes to
    // the file instead of marking the region as sparse zero area.
    DWORD dwTemp;
    DeviceIoControl(hSparseFile, FSCTL_SET_SPARSE, NULL, 0, NULL, 0, &dwTemp, NULL);

    return hSparseFile;
}

/*!
 * Converting a file region to A sparse zero area.
 *
 * \param hSparseFile
 * Handle of the sparse file
 *
 * \param start
 * Start address of the sparse zero area
 *
 * \param size
 * Size of the sparse zero block. The minimum sparse size is 64KB.
 *
 * \remarks
 * Note that SetSparseRange does not perform actual file I/O, and unlike the
 * WriteFile function, it does not move the current file I/O pointer or sets
 * the end-of-file pointer. That is, if you want to place a sparse zero block
 * in the end of the file, you must move the file pointer accordingly using
 * the FileStream.Seek function, otherwise DeviceIoControl will have no effect
 */
void SetSparseRange(HANDLE hSparseFile, LONGLONG start, LONGLONG size)
{
    // Specify the starting and the ending address (not the size) of the
    // sparse zero block
    FILE_ZERO_DATA_INFORMATION fzdi;
    fzdi.FileOffset.QuadPart = start;
    fzdi.BeyondFinalZero.QuadPart = start + size;

    // Mark the range as sparse zero block
    DWORD dwTemp;
    DeviceIoControl(hSparseFile, FSCTL_SET_ZERO_DATA, &fzdi, sizeof(fzdi), NULL, 0, &dwTemp, NULL);
}

/*!
 * Query the sparse file layout.
 *
 * \param lpFileName
 * File name
 */
BOOL GetSparseRanges(LPCTSTR lpFileName)
{
    // Open the file for read
    HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    LARGE_INTEGER liFileSize;
    GetFileSizeEx(hFile, &liFileSize);

    // Range to be examined (the whole file)
    FILE_ALLOCATED_RANGE_BUFFER queryRange;
    queryRange.FileOffset.QuadPart = 0;
    queryRange.Length = liFileSize;

    // Allocated areas info
    FILE_ALLOCATED_RANGE_BUFFER allocRanges[1024];

    DWORD nbytes;
    BOOL fFinished;
    _putws(L"\nAllocated ranges in the file:");
    do
    {
        fFinished = DeviceIoControl(hFile,
                                    FSCTL_QUERY_ALLOCATED_RANGES,
                                    &queryRange,
                                    sizeof(queryRange),
                                    allocRanges,
                                    sizeof(allocRanges),
                                    &nbytes,
                                    NULL);

        if (!fFinished)
        {
            DWORD dwError = GetLastError();

            // ERROR_MORE_DATA is the only error that is normal
            if (dwError != ERROR_MORE_DATA)
            {
                wprintf(L"DeviceIoControl failed w/err 0x%08lx\n", dwError);
                CloseHandle(hFile);
                return FALSE;
            }
        }

        // Calculate the number of records returned
        DWORD dwAllocRangeCount = nbytes / sizeof(FILE_ALLOCATED_RANGE_BUFFER);

        // Print each allocated range
        for (DWORD i = 0; i < dwAllocRangeCount; i++)
        {
            wprintf(L"allocated range: [%I64u] [%I64u]\n",
                    allocRanges[i].FileOffset.QuadPart,
                    allocRanges[i].Length.QuadPart);
        }

        // Set starting address and size for the next query
        if (!fFinished && dwAllocRangeCount > 0)
        {
            queryRange.FileOffset.QuadPart = allocRanges[dwAllocRangeCount - 1].FileOffset.QuadPart +
                                             allocRanges[dwAllocRangeCount - 1].Length.QuadPart;

            queryRange.Length.QuadPart = liFileSize.QuadPart - queryRange.FileOffset.QuadPart;
        }

    } while (!fFinished);

    CloseHandle(hFile);
    return TRUE;
}
