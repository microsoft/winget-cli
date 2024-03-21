/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BIT7ZLIBRARY_HPP
#define BIT7ZLIBRARY_HPP

#include <string>

#include "bitformat.hpp"
#include "bittypes.hpp"
#include "bitwindows.hpp"

//! @cond IGNORE_BLOCK_IN_DOXYGEN
struct IInArchive;
struct IOutArchive;

template< typename T >
class CMyComPtr;
//! @endcond

/**
 * @brief The main namespace of the bit7z library.
 */
namespace bit7z {

/**
 * @brief The default file path for the 7-zip shared library to be used by bit7z
 * in case the user doesn't pass a path to the constructor of the Bit7zLibrary class.
 *
 * @note On Windows, the default library is 7z.dll, and it is searched following the Win32 API rules
 * (https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order).
 *
 * @note On Linux, the default library is the absolute path to the "7z.so" installed by p7zip.
 *
 * @note In all other cases, the value will be the relative path to a "7z.so" in the working directory of the program.
 */
#ifdef __DOXYGEN__
constexpr auto kDefaultLibrary = "<platform-dependent value>";
#elif defined( _WIN32 )
constexpr auto kDefaultLibrary = BIT7Z_STRING( "7z.dll" );
#elif defined( __linux__ )
constexpr auto kDefaultLibrary = "/usr/lib/p7zip/7z.so"; // Default installation path of the p7zip shared library.
#else
constexpr auto kDefaultLibrary = "./7z.so";
#endif

/**
 * @brief The Bit7zLibrary class allows accessing the basic functionalities provided by the 7z DLLs.
 */
class Bit7zLibrary final {
    public:
        Bit7zLibrary( const Bit7zLibrary& ) = delete;

        Bit7zLibrary( Bit7zLibrary&& ) = delete;

        auto operator=( const Bit7zLibrary& ) -> Bit7zLibrary& = delete;

        auto operator=( Bit7zLibrary&& ) -> Bit7zLibrary& = delete;

        /**
         * @brief Constructs a Bit7zLibrary object by loading the specified 7zip shared library.
         *
         * By default, it searches a 7z.dll in the same path of the application.
         *
         * @param libraryPath  the path to the shared library file to be loaded.
         */
        explicit Bit7zLibrary( const tstring& libraryPath = kDefaultLibrary );

        /**
         * @brief Destructs the Bit7zLibrary object, freeing the loaded shared library.
         */
        ~Bit7zLibrary();

        /**
         * @brief Set the 7-zip shared library to use large memory pages.
         */
        void setLargePageMode();

    private:
        HMODULE mLibrary;
        FARPROC mCreateObjectFunc;

        BIT7Z_NODISCARD
        auto initInArchive( const BitInFormat& format ) const -> CMyComPtr< IInArchive >;

        BIT7Z_NODISCARD
        auto initOutArchive( const BitInOutFormat& format ) const -> CMyComPtr< IOutArchive >;

        friend class BitInputArchive;
        friend class BitOutputArchive;
};

}  // namespace bit7z

#endif // BIT7ZLIBRARY_HPP
