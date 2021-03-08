/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * CppSparseFile.h : defines various apis for creation and access of sparse files under windows
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma region Includes
#include <assert.h>
#include <stdio.h>
#include <tchar.h>

#include <windows.h>
#pragma endregion

/*!
 * VolumeSupportsSparseFiles determines if the volume supports sparse streams.
 *
 * \param lpRootPathName
 * Volume root path e.g. C:\
 */
BOOL VolumeSupportsSparseFiles(LPCTSTR lpRootPathName);

/*!
 * IsSparseFile determines if a file is sparse.
 *
 * \param lpFileName
 * File name
 */
BOOL IsSparseFile(LPCTSTR lpFileName);

/*!
 * Get sparse file sizes.
 *
 * \param lpFileName
 * File name
 *
 * \see
 * http://msdn.microsoft.com/en-us/library/aa365276.aspx
 */
BOOL GetSparseFileSize(LPCTSTR lpFileName);

/*!
 * Create a sparse file.
 *
 * \param lpFileName
 * The name of the sparse file
 */
HANDLE CreateSparseFile(LPCTSTR lpFileName);

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
void SetSparseRange(HANDLE hSparseFile, LONGLONG start, LONGLONG size);

/*!
 * Query the sparse file layout.
 *
 * \param lpFileName
 * File name
 */
BOOL GetSparseRanges(LPCTSTR lpFileName);
