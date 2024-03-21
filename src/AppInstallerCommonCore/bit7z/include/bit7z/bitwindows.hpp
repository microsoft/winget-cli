/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITWINDOWS_HPP
#define BITWINDOWS_HPP

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <propidl.h>
#else

/* We don't have the "Windows.h" header on Unix systems, so in theory, we could use the "MyWindows.h" of p7zip/7-zip.
 * However, some of bit7z's public API headers need some Win32 API structs like PROPVARIANT and GUID.
 * Hence, it would result in the leak of p7zip/7-zip headers, making bit7z's clients dependent on them.
 * Also, (publicly) forward declaring them and then (internally) using the "MyWindows.h" is impossible:
 * the two different declarations would conflict, making the compilation fail.
 *
 * To avoid all these issues, we define the required Win32 API structs, constants, and type aliases,
 * with the same definitions in the MyWindows.h header.
 * We will use only this header and avoid including "MyWindows.h" or similar headers (e.g., StdAfx.h). */
#include <cerrno>
#include <cstdint>
#include <cstddef>

// Avoiding accidentally including p7zip's MyWindows.h, so that its inclusion is not needed in client code!
#ifndef __MYWINDOWS_H
#define __MYWINDOWS_H // NOLINT
#endif

// Avoiding accidentally including 7-zip's MyWindows.h, so that its inclusion is not needed in client code!
#ifndef __MY_WINDOWS_H
#define __MY_WINDOWS_H // NOLINT
#endif

// Avoiding accidentally including 7-zip's MyWindows.h, so that its inclusion is not needed in client code!
#ifndef ZIP7_INC_MY_WINDOWS_H // 7-zip 23.01+
#define ZIP7_INC_MY_WINDOWS_H
#endif

using std::size_t;

#define WINAPI

namespace bit7z {

// Win32 type aliases
using FARPROC = void*;
using HMODULE = void*;
using HRESULT = int;
using OLECHAR = wchar_t;
using BSTR = OLECHAR*;
using VARIANT_BOOL = short;
using VARTYPE = unsigned short;

using WORD = unsigned short;
using DWORD = unsigned int;

using ULONG = unsigned int;
using PROPID = ULONG;

// Error codes constants can be useful for bit7z's clients on Unix (since they don't have the Windows.h header).

#ifndef S_OK // Silencing cppcheck warning on E_NOTIMPL, probably a bug of cppcheck.
// Win32 HRESULT error codes.
constexpr auto S_OK = static_cast< HRESULT >( 0x00000000L );
constexpr auto S_FALSE = static_cast< HRESULT >( 0x00000001L );
constexpr auto E_NOTIMPL = static_cast< HRESULT >( 0x80004001L );
constexpr auto E_NOINTERFACE = static_cast< HRESULT >( 0x80004002L );
constexpr auto E_ABORT = static_cast< HRESULT >( 0x80004004L );
constexpr auto E_FAIL = static_cast< HRESULT >( 0x80004005L );
constexpr auto STG_E_INVALIDFUNCTION = static_cast< HRESULT >( 0x80030001L );
constexpr auto E_OUTOFMEMORY = static_cast< HRESULT >( 0x8007000EL );
constexpr auto E_INVALIDARG = static_cast< HRESULT >( 0x80070057L );
#endif

#ifndef ERROR_ALREADY_EXISTS
// Win32 error codes (defined by both p7zip and 7-zip as equivalent to POSIX error codes).
constexpr auto ERROR_ALREADY_EXISTS = EEXIST;
constexpr auto ERROR_DISK_FULL = ENOSPC;
constexpr auto ERROR_FILE_EXISTS = EEXIST;
constexpr auto ERROR_FILE_NOT_FOUND = ENOENT;
constexpr auto ERROR_INVALID_PARAMETER = EINVAL;
constexpr auto ERROR_INVALID_FUNCTION = EINVAL;
constexpr auto ERROR_INVALID_HANDLE = EBADF;
constexpr auto ERROR_OPEN_FAILED = EIO;
constexpr auto ERROR_PATH_NOT_FOUND = ENOENT;
constexpr auto ERROR_SEEK = EIO;
constexpr auto ERROR_READ_FAULT = EIO;
constexpr auto ERROR_WRITE_FAULT = EIO;

// Win32 error codes (defined by p7zip with the same values as in Windows API).
constexpr auto ERROR_NO_MORE_FILES = 0x100018;
constexpr auto ERROR_DIRECTORY = 267;
#endif

// Win32 structs.
struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

struct LARGE_INTEGER {
    int64_t QuadPart;
};

struct ULARGE_INTEGER {
    uint64_t QuadPart;
};

struct PROPVARIANT {
    VARTYPE vt;
    WORD wReserved1;
    WORD wReserved2;
    WORD wReserved3;
    union {
        char cVal;
        unsigned char bVal;
        short iVal;
        unsigned short uiVal;
        int lVal;
        unsigned int ulVal;
        int intVal;
        unsigned int uintVal;
        LARGE_INTEGER hVal;
        ULARGE_INTEGER uhVal;
        VARIANT_BOOL boolVal;
        int scode;
        FILETIME filetime;
        BSTR bstrVal;
    };
};

}  // namespace bit7z

#endif

#endif //BITWINDOWS_HPP
