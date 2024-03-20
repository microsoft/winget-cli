/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITTYPES_HPP
#define BITTYPES_HPP

#include <string>
#include <vector>

// Must be included here since the user might have manually enabled a BIT7Z_* compilation option
// by uncommenting the corresponding macro define in bitdefines.hpp.
#include "bitdefines.hpp"

#ifdef BIT7Z_REGEX_MATCHING
#include <regex>
#endif

namespace bit7z {

/**
 * @brief A type representing a byte.
 */
#ifdef BIT7Z_USE_STD_BYTE
#if __cpp_lib_byte
using byte_t = std::byte;
#else
enum class byte_t : unsigned char {}; //same as std::byte_t
#endif
#else
using byte_t = unsigned char;
#endif

/** @cond */
using buffer_t = std::vector< byte_t >;
using index_t = std::ptrdiff_t; //like gsl::index (https://github.com/microsoft/GSL)

template< class Char >
struct StringTraits;

template<>
struct StringTraits< char > {
    template< class T >
    static inline auto convertToString( T value ) -> std::string {
        return std::to_string( value );
    }
};

template<>
struct StringTraits< wchar_t > {
    template< class T >
    static inline auto convertToString( T value ) -> std::wstring {
        return std::to_wstring( value );
    }
};
/** @endcond */

/**
 * Native string type of the system.
 * @note On Windows, it is an alias of `std::wstring`.
 */
#ifdef _WIN32
using native_string = std::wstring;
#define BIT7Z_NATIVE_STRING_( str ) L##str
#define BIT7Z_NATIVE_STRING( str ) BIT7Z_NATIVE_STRING_( str )
#else
using native_string = std::string;
#define BIT7Z_NATIVE_STRING( str ) str
#endif

/**
 * @note On Windows, if the `BIT7Z_USE_NATIVE_STRING` option is enabled, `tchar` is an alias of `wchar_t`.
 */
#if defined( BIT7Z_USE_NATIVE_STRING ) && defined( _WIN32 ) // Windows with native strings
using tchar = wchar_t;
#define BIT7Z_STRING( str ) BIT7Z_NATIVE_STRING_( str )
#else // Unix, and Windows with non-native strings
using tchar = char;
#define BIT7Z_STRING( str ) str
#endif

/**
 * @note On Windows, if the `BIT7Z_USE_NATIVE_STRING` option is enabled, `tstring` is an alias for std::wstring.
 * Otherwise, it is an alias for std::string (default).
 */
using tstring = std::basic_string< tchar >;

#ifdef BIT7Z_REGEX_MATCHING
/**
 * @note On Windows, if the `BIT7Z_USE_NATIVE_STRING` option is enabled, `tregex` is an alias for std::wregex.
 * Otherwise, it is an alias for std::regex (default).
 */
using tregex = std::basic_regex< tchar >;
#endif

template< typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type >
inline auto to_tstring( T arg ) -> std::basic_string< tchar > {
    return StringTraits< tchar >::convertToString( arg );
}

/**
 * Converts a native string to a tstring.
 *
 * @note On Linux or on Windows when BIT7Z_USE_NATIVE_STRING is used,
 * both native_string and tstring are aliases of the same string type;
 * in this case, no conversion is performed, and a const reference to the original string is returned.
 *
 * @param str   The native string to be converted.
 *
 * @return the converted tstring.
 */
#if defined( _WIN32 ) && !defined( BIT7Z_USE_NATIVE_STRING )
auto to_tstring( const native_string& str ) -> tstring;
#else
auto to_tstring( const native_string& str ) -> const tstring&;
#endif

}  // namespace bit7z

#endif // BITTYPES_HPP
