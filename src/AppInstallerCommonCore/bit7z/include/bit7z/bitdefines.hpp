/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITDEFINES_HPP
#define BITDEFINES_HPP

/* Uncomment the following macros if you don't want to define them yourself in your project files,
 * and you can't enable them via CMake. */
//#define BIT7Z_AUTO_FORMAT
//#define BIT7Z_AUTO_PREFIX_LONG_PATHS
//#define BIT7Z_DISABLE_USE_STD_FILESYSTEM
//#define BIT7Z_REGEX_MATCHING
//#define BIT7Z_USE_STD_BYTE
//#define BIT7Z_USE_NATIVE_STRING

#if ( defined( _MSVC_LANG ) && _MSVC_LANG >= 201703L ) || ( defined( __cplusplus ) && __cplusplus >= 201703L )
#   define BIT7Z_CPP_STANDARD 17
#elif ( defined( _MSVC_LANG ) && _MSVC_LANG >= 201402L ) || ( defined( __cplusplus ) && __cplusplus >= 201402L )
#   define BIT7Z_CPP_STANDARD 14
#else
#   define BIT7Z_CPP_STANDARD 11
#endif

#ifndef BIT7Z_DISABLE_USE_STD_FILESYSTEM
#   if defined( __cpp_lib_filesystem )
#       define BIT7Z_USE_STANDARD_FILESYSTEM
#   elif BIT7Z_CPP_STANDARD >= 17 && defined( __has_include )
#       if __has_include( <filesystem> )
#           define BIT7Z_USE_STANDARD_FILESYSTEM
#       endif
#   endif
#endif

/* Macro defines for [[nodiscard]] and [[maybe_unused]] attributes. */
#if defined( __has_cpp_attribute )
#   if __has_cpp_attribute( nodiscard )
#       define BIT7Z_NODISCARD [[nodiscard]]
#   endif
#   if __has_cpp_attribute( maybe_unused )
#       define BIT7Z_MAYBE_UNUSED [[maybe_unused]]
#   endif
#   if __has_cpp_attribute( deprecated )
#       define BIT7Z_DEPRECATED [[deprecated]]
#       define BIT7Z_DEPRECATED_MSG( msg ) [[deprecated( msg )]]
#   endif
#endif

/* The compiler doesn't support __has_cpp_attribute, but it is using the C++17 standard. */
#if !defined( BIT7Z_NODISCARD ) && BIT7Z_CPP_STANDARD >= 17
#   define BIT7Z_NODISCARD [[nodiscard]]
#endif

#if !defined( BIT7Z_MAYBE_UNUSED ) && BIT7Z_CPP_STANDARD >= 17
#   define BIT7Z_MAYBE_UNUSED [[maybe_unused]]
#endif

#if !defined( BIT7Z_DEPRECATED ) && BIT7Z_CPP_STANDARD >= 14
#   define BIT7Z_DEPRECATED [[deprecated]]
#   define BIT7Z_DEPRECATED_MSG( msg ) [[deprecated( msg )]]
#endif

/* Compiler is using at most the C++14 standard, so we use the compiler-specific attributes/defines were possible. */
#ifndef BIT7Z_NODISCARD
#   if defined( __GNUC__ ) || defined(__clang__)
#       define BIT7Z_NODISCARD __attribute__(( warn_unused_result ))
#   elif defined( _Check_return_ ) // Old MSVC versions
#       define BIT7Z_NODISCARD _Check_return_
#   else
#       define BIT7Z_NODISCARD
#   endif
#endif
#ifndef BIT7Z_MAYBE_UNUSED
#   if defined( __GNUC__ ) || defined(__clang__)
#       define BIT7Z_MAYBE_UNUSED __attribute__(( unused ))
#   else
#       define BIT7Z_MAYBE_UNUSED
#   endif
#endif

/* Compiler is using the C++11 standard, so we use the compiler-specific attributes were possible.
 * Note: these macros are used in the public API, so we cannot assume that we are always using a C++14 compiler.*/
#ifndef BIT7Z_DEPRECATED
#   if defined( __GNUC__ ) || defined( __clang__ )
#       define BIT7Z_DEPRECATED __attribute__(( __deprecated__ ))
#       define BIT7Z_DEPRECATED_MSG( msg ) __attribute__(( __deprecated__( msg ) ))
#   elif defined( _MSC_VER )
#       define BIT7Z_DEPRECATED __declspec( deprecated )
#       define BIT7Z_DEPRECATED_MSG( msg ) __declspec( deprecated( msg ) )
#   else
#       define BIT7Z_DEPRECATED
#       define BIT7Z_DEPRECATED_MSG( msg )
#   endif
#endif

#ifndef BIT7Z_DEPRECATED_ENUMERATOR
// Before v6.0, GCC didn't support deprecating single enumerators.
#   if defined( __GNUC__ ) && !defined( __clang__ ) && __GNUC__ < 6
#       define BIT7Z_DEPRECATED_ENUMERATOR( deprecated_value, new_value, msg ) deprecated_value = new_value
#   else
#       define BIT7Z_DEPRECATED_ENUMERATOR( deprecated_value, new_value, msg ) \
                deprecated_value BIT7Z_DEPRECATED_MSG( msg ) = new_value
#   endif
#endif

#ifndef BIT7Z_DEPRECATED_TYPEDEF
#   if defined( __GNUC__ ) && !defined( __clang__ ) && __GNUC__ < 7
#       define BIT7Z_DEPRECATED_TYPEDEF( alias_name, alias_value, msg ) \
                using alias_name BIT7Z_MAYBE_UNUSED __attribute__(( __deprecated__( msg ) )) = alias_value
#   else
#       define BIT7Z_DEPRECATED_TYPEDEF( alias_name, alias_value, msg ) \
                using alias_name BIT7Z_MAYBE_UNUSED BIT7Z_DEPRECATED_MSG( msg ) = alias_value
#   endif
#endif

#endif //BITDEFINES_HPP
