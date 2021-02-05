// Copyright 2007-2010 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef JSON_CONFIG_H_INCLUDED
#define JSON_CONFIG_H_INCLUDED

/// If defined, indicates that json library is embedded in CppTL library.
//# define JSON_IN_CPPTL 1

/// If defined, indicates that json may leverage CppTL library
//#  define JSON_USE_CPPTL 1
/// If defined, indicates that cpptl vector based map should be used instead of
/// std::map
/// as Value container.
//#  define JSON_USE_CPPTL_SMALLMAP 1

// If non-zero, the library uses exceptions to report bad input instead of C
// assertion macros. The default is to use exceptions.
#ifndef JSON_USE_EXCEPTION
#define JSON_USE_EXCEPTION 1
#endif

/// If defined, indicates that the source file is amalgated
/// to prevent private header inclusion.
/// Remarks: it is automatically defined in the generated amalgated header.
// #define JSON_IS_AMALGAMATION

#ifdef JSON_IN_CPPTL
#include <cpptl/config.h>
#ifndef JSON_USE_CPPTL
#define JSON_USE_CPPTL 1
#endif
#endif

#ifdef JSON_IN_CPPTL
#define JSON_API CPPTL_API
#elif defined(JSON_DLL_BUILD)
#if defined(_MSC_VER)
#define JSON_API __declspec(dllexport)
#define JSONCPP_DISABLE_DLL_INTERFACE_WARNING
#endif // if defined(_MSC_VER)
#elif defined(JSON_DLL)
#if defined(_MSC_VER)
#define JSON_API __declspec(dllimport)
#define JSONCPP_DISABLE_DLL_INTERFACE_WARNING
#endif // if defined(_MSC_VER)
#endif // ifdef JSON_IN_CPPTL
#if !defined(JSON_API)
#define JSON_API
#endif

// If JSON_NO_INT64 is defined, then Json only support C++ "int" type for
// integer
// Storages, and 64 bits integer support is disabled.
// #define JSON_NO_INT64 1

#if defined(_MSC_VER) && _MSC_VER <= 1200 // MSVC 6
// Microsoft Visual Studio 6 only support conversion from __int64 to double
// (no conversion from unsigned __int64).
#define JSON_USE_INT64_DOUBLE_CONVERSION 1
// Disable warning 4786 for VS6 caused by STL (identifier was truncated to '255'
// characters in the debug information)
// All projects I've ever seen with VS6 were using this globally (not bothering
// with pragma push/pop).
#pragma warning(disable : 4786)
#endif // if defined(_MSC_VER)  &&  _MSC_VER < 1200 // MSVC 6

#if defined(_MSC_VER) && _MSC_VER >= 1500 // MSVC 2008
/// Indicates that the following function is deprecated.
#define JSONCPP_DEPRECATED(message) __declspec(deprecated(message))
#endif

#if !defined(JSONCPP_DEPRECATED)
#define JSONCPP_DEPRECATED(message)
#endif // if !defined(JSONCPP_DEPRECATED)

namespace Json {
typedef int Int;
typedef unsigned int UInt;
#if defined(JSON_NO_INT64)
typedef int LargestInt;
typedef unsigned int LargestUInt;
#undef JSON_HAS_INT64
#else                 // if defined(JSON_NO_INT64)
// For Microsoft Visual use specific types as long long is not supported
#if defined(_MSC_VER) // Microsoft Visual Studio
typedef __int64 Int64;
typedef unsigned __int64 UInt64;
#else                 // if defined(_MSC_VER) // Other platforms, use long long
typedef long long int Int64;
typedef unsigned long long int UInt64;
#endif // if defined(_MSC_VER)
typedef Int64 LargestInt;
typedef UInt64 LargestUInt;
#define JSON_HAS_INT64
#endif // if defined(JSON_NO_INT64)
} // end namespace Json

#endif // JSON_CONFIG_H_INCLUDED
