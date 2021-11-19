// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// Google Test - The Google C++ Testing and Mocking Framework
//
// This file implements a universal value printer that can print a
// value of any type T:
//
//   void ::testing::internal::UniversalPrinter<T>::Print(value, ostream_ptr);
//
// A user can teach this function how to print a class type T by
// defining either operator<<() or PrintTo() in the namespace that
// defines T.  More specifically, the FIRST defined function in the
// following list will be used (assuming T is defined in namespace
// foo):
//
//   1. foo::PrintTo(const T&, ostream*)
//   2. operator<<(ostream&, const T&) defined in either foo or the
//      global namespace.
//
// However if T is an STL-style container then it is printed element-wise
// unless foo::PrintTo(const T&, ostream*) is defined. Note that
// operator<<() is ignored for container types.
//
// If none of the above is defined, it will print the debug string of
// the value if it is a protocol buffer, or print the raw bytes in the
// value otherwise.
//
// To aid debugging: when T is a reference type, the address of the
// value is also printed; when T is a (const) char pointer, both the
// pointer value and the NUL-terminated string it points to are
// printed.
//
// We also provide some convenient wrappers:
//
//   // Prints a value to a string.  For a (const or not) char
//   // pointer, the NUL-terminated string (but not the pointer) is
//   // printed.
//   std::string ::testing::PrintToString(const T& value);
//
//   // Prints a value tersely: for a reference type, the referenced
//   // value (but not the address) is printed; for a (const or not) char
//   // pointer, the NUL-terminated string (but not the pointer) is
//   // printed.
//   void ::testing::internal::UniversalTersePrint(const T& value, ostream*);
//
//   // Prints value using the type inferred by the compiler.  The difference
//   // from UniversalTersePrint() is that this function prints both the
//   // pointer and the NUL-terminated string for a (const or not) char pointer.
//   void ::testing::internal::UniversalPrint(const T& value, ostream*);
//
//   // Prints the fields of a tuple tersely to a string vector, one
//   // element for each field. Tuple support must be enabled in
//   // gtest-port.h.
//   std::vector<string> UniversalTersePrintTupleFieldsToStrings(
//       const Tuple& value);
//
// Known limitation:
//
// The print primitives print the elements of an STL-style container
// using the compiler-inferred type of *iter where iter is a
// const_iterator of the container.  When const_iterator is an input
// iterator but not a forward iterator, this inferred type may not
// match value_type, and the print output may be incorrect.  In
// practice, this is rarely a problem as for most containers
// const_iterator is a forward iterator.  We'll fix this if there's an
// actual need for it.  Note that this fix cannot rely on value_type
// being defined as many user-defined container types don't have
// value_type.

// GOOGLETEST_CM0001 DO NOT DELETE

#ifndef GOOGLETEST_INCLUDE_GTEST_GTEST_PRINTERS_H_
#define GOOGLETEST_INCLUDE_GTEST_GTEST_PRINTERS_H_

#include <functional>
#include <memory>
#include <ostream>  // NOLINT
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "gtest/internal/gtest-internal.h"
#include "gtest/internal/gtest-port.h"

namespace testing {

// Definitions in the internal* namespaces are subject to change without notice.
// DO NOT USE THEM IN USER CODE!
namespace internal {

template <typename T>
void UniversalPrint(const T& value, ::std::ostream* os);

// Used to print an STL-style container when the user doesn't define
// a PrintTo() for it.
struct ContainerPrinter {
  template <typename T,
            typename = typename std::enable_if<
                (sizeof(IsContainerTest<T>(0)) == sizeof(IsContainer)) &&
                !IsRecursiveContainer<T>::value>::type>
  static void PrintValue(const T& container, std::ostream* os) {
    const size_t kMaxCount = 32;  // The maximum number of elements to print.
    *os << '{';
    size_t count = 0;
    for (auto&& elem : container) {
      if (count > 0) {
        *os << ',';
        if (count == kMaxCount) {  // Enough has been printed.
          *os << " ...";
          break;
        }
      }
      *os << ' ';
      // We cannot call PrintTo(elem, os) here as PrintTo() doesn't
      // handle `elem` being a native array.
      internal::UniversalPrint(elem, os);
      ++count;
    }

    if (count > 0) {
      *os << ' ';
    }
    *os << '}';
  }
};

// Used to print a pointer that is neither a char pointer nor a member
// pointer, when the user doesn't define PrintTo() for it.  (A member
// variable pointer or member function pointer doesn't really point to
// a location in the address space.  Their representation is
// implementation-defined.  Therefore they will be printed as raw
// bytes.)
struct FunctionPointerPrinter {
  template <typename T, typename = typename std::enable_if<
                            std::is_function<T>::value>::type>
  static void PrintValue(T* p, ::std::ostream* os) {
    if (p == nullptr) {
      *os << "NULL";
    } else {
      // T is a function type, so '*os << p' doesn't do what we want
      // (it just prints p as bool).  We want to print p as a const
      // void*.
      *os << reinterpret_cast<const void*>(p);
    }
  }
};

struct PointerPrinter {
  template <typename T>
  static void PrintValue(T* p, ::std::ostream* os) {
    if (p == nullptr) {
      *os << "NULL";
    } else {
      // T is not a function type.  We just call << to print p,
      // relying on ADL to pick up user-defined << for their pointer
      // types, if any.
      *os << p;
    }
  }
};

namespace internal_stream_operator_without_lexical_name_lookup {

// The presence of an operator<< here will terminate lexical scope lookup
// straight away (even though it cannot be a match because of its argument
// types). Thus, the two operator<< calls in StreamPrinter will find only ADL
// candidates.
struct LookupBlocker {};
void operator<<(LookupBlocker, LookupBlocker);

struct StreamPrinter {
  template <typename T,
            // Don't accept member pointers here. We'd print them via implicit
            // conversion to bool, which isn't useful.
            typename = typename std::enable_if<
                !std::is_member_pointer<T>::value>::type,
            // Only accept types for which we can find a streaming operator via
            // ADL (possibly involving implicit conversions).
            typename = decltype(std::declval<std::ostream&>()
                                << std::declval<const T&>())>
  static void PrintValue(const T& value, ::std::ostream* os) {
    // Call streaming operator found by ADL, possibly with implicit conversions
    // of the arguments.
    *os << value;
  }
};

}  // namespace internal_stream_operator_without_lexical_name_lookup

struct ProtobufPrinter {
  // We print a protobuf using its ShortDebugString() when the string
  // doesn't exceed this many characters; otherwise we print it using
  // DebugString() for better readability.
  static const size_t kProtobufOneLinerMaxLength = 50;

  template <typename T,
            typename = typename std::enable_if<
                internal::HasDebugStringAndShortDebugString<T>::value>::type>
  static void PrintValue(const T& value, ::std::ostream* os) {
    std::string pretty_str = value.ShortDebugString();
    if (pretty_str.length() > kProtobufOneLinerMaxLength) {
      pretty_str = "\n" + value.DebugString();
    }
    *os << ("<" + pretty_str + ">");
  }
};

struct ConvertibleToIntegerPrinter {
  // Since T has no << operator or PrintTo() but can be implicitly
  // converted to BiggestInt, we print it as a BiggestInt.
  //
  // Most likely T is an enum type (either named or unnamed), in which
  // case printing it as an integer is the desired behavior.  In case
  // T is not an enum, printing it as an integer is the best we can do
  // given that it has no user-defined printer.
  static void PrintValue(internal::BiggestInt value, ::std::ostream* os) {
    *os << value;
  }
};

struct ConvertibleToStringViewPrinter {
#if GTEST_INTERNAL_HAS_STRING_VIEW
  static void PrintValue(internal::StringView value, ::std::ostream* os) {
    internal::UniversalPrint(value, os);
  }
#endif
};


// Prints the given number of bytes in the given object to the given
// ostream.
GTEST_API_ void PrintBytesInObjectTo(const unsigned char* obj_bytes,
                                     size_t count,
                                     ::std::ostream* os);
struct RawBytesPrinter {
  // SFINAE on `sizeof` to make sure we have a complete type.
  template <typename T, size_t = sizeof(T)>
  static void PrintValue(const T& value, ::std::ostream* os) {
    PrintBytesInObjectTo(
        static_cast<const unsigned char*>(
            // Load bearing cast to void* to support iOS
            reinterpret_cast<const void*>(std::addressof(value))),
        sizeof(value), os);
  }
};

struct FallbackPrinter {
  template <typename T>
  static void PrintValue(const T&, ::std::ostream* os) {
    *os << "(incomplete type)";
  }
};

// Try every printer in order and return the first one that works.
template <typename T, typename E, typename Printer, typename... Printers>
struct FindFirstPrinter : FindFirstPrinter<T, E, Printers...> {};

template <typename T, typename Printer, typename... Printers>
struct FindFirstPrinter<
    T, decltype(Printer::PrintValue(std::declval<const T&>(), nullptr)),
    Printer, Printers...> {
  using type = Printer;
};

// Select the best printer in the following order:
//  - Print containers (they have begin/end/etc).
//  - Print function pointers.
//  - Print object pointers.
//  - Use the stream operator, if available.
//  - Print protocol buffers.
//  - Print types convertible to BiggestInt.
//  - Print types convertible to StringView, if available.
//  - Fallback to printing the raw bytes of the object.
template <typename T>
void PrintWithFallback(const T& value, ::std::ostream* os) {
  using Printer = typename FindFirstPrinter<
      T, void, ContainerPrinter, FunctionPointerPrinter, PointerPrinter,
      internal_stream_operator_without_lexical_name_lookup::StreamPrinter,
      ProtobufPrinter, ConvertibleToIntegerPrinter,
      ConvertibleToStringViewPrinter, RawBytesPrinter, FallbackPrinter>::type;
  Printer::PrintValue(value, os);
}

// FormatForComparison<ToPrint, OtherOperand>::Format(value) formats a
// value of type ToPrint that is an operand of a comparison assertion
// (e.g. ASSERT_EQ).  OtherOperand is the type of the other operand in
// the comparison, and is used to help determine the best way to
// format the value.  In particular, when the value is a C string
// (char pointer) and the other operand is an STL string object, we
// want to format the C string as a string, since we know it is
// compared by value with the string object.  If the value is a char
// pointer but the other operand is not an STL string object, we don't
// know whether the pointer is supposed to point to a NUL-terminated
// string, and thus want to print it as a pointer to be safe.
//
// INTERNAL IMPLEMENTATION - DO NOT USE IN A USER PROGRAM.

// The default case.
template <typename ToPrint, typename OtherOperand>
class FormatForComparison {
 public:
  static ::std::string Format(const ToPrint& value) {
    return ::testing::PrintToString(value);
  }
};

// Array.
template <typename ToPrint, size_t N, typename OtherOperand>
class FormatForComparison<ToPrint[N], OtherOperand> {
 public:
  static ::std::string Format(const ToPrint* value) {
    return FormatForComparison<const ToPrint*, OtherOperand>::Format(value);
  }
};

// By default, print C string as pointers to be safe, as we don't know
// whether they actually point to a NUL-terminated string.

#define GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(CharType)                \
  template <typename OtherOperand>                                      \
  class FormatForComparison<CharType*, OtherOperand> {                  \
   public:                                                              \
    static ::std::string Format(CharType* value) {                      \
      return ::testing::PrintToString(static_cast<const void*>(value)); \
    }                                                                   \
  }

GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(char);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(const char);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(wchar_t);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(const wchar_t);
#ifdef __cpp_char8_t
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(char8_t);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(const char8_t);
#endif
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(char16_t);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(const char16_t);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(char32_t);
GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_(const char32_t);

#undef GTEST_IMPL_FORMAT_C_STRING_AS_POINTER_

// If a C string is compared with an STL string object, we know it's meant
// to point to a NUL-terminated string, and thus can print it as a string.

#define GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(CharType, OtherStringType) \
  template <>                                                           \
  class FormatForComparison<CharType*, OtherStringType> {               \
   public:                                                              \
    static ::std::string Format(CharType* value) {                      \
      return ::testing::PrintToString(value);                           \
    }                                                                   \
  }

GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(char, ::std::string);
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(const char, ::std::string);
#ifdef __cpp_char8_t
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(char8_t, ::std::u8string);
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(const char8_t, ::std::u8string);
#endif
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(char16_t, ::std::u16string);
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(const char16_t, ::std::u16string);
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(char32_t, ::std::u32string);
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(const char32_t, ::std::u32string);

#if GTEST_HAS_STD_WSTRING
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(wchar_t, ::std::wstring);
GTEST_IMPL_FORMAT_C_STRING_AS_STRING_(const wchar_t, ::std::wstring);
#endif

#undef GTEST_IMPL_FORMAT_C_STRING_AS_STRING_

// Formats a comparison assertion (e.g. ASSERT_EQ, EXPECT_LT, and etc)
// operand to be used in a failure message.  The type (but not value)
// of the other operand may affect the format.  This allows us to
// print a char* as a raw pointer when it is compared against another
// char* or void*, and print it as a C string when it is compared
// against an std::string object, for example.
//
// INTERNAL IMPLEMENTATION - DO NOT USE IN A USER PROGRAM.
template <typename T1, typename T2>
std::string FormatForComparisonFailureMessage(
    const T1& value, const T2& /* other_operand */) {
  return FormatForComparison<T1, T2>::Format(value);
}

// UniversalPrinter<T>::Print(value, ostream_ptr) prints the given
// value to the given ostream.  The caller must ensure that
// 'ostream_ptr' is not NULL, or the behavior is undefined.
//
// We define UniversalPrinter as a class template (as opposed to a
// function template), as we need to partially specialize it for
// reference types, which cannot be done with function templates.
template <typename T>
class UniversalPrinter;

// Prints the given value using the << operator if it has one;
// otherwise prints the bytes in it.  This is what
// UniversalPrinter<T>::Print() does when PrintTo() is not specialized
// or overloaded for type T.
//
// A user can override this behavior for a class type Foo by defining
// an overload of PrintTo() in the namespace where Foo is defined.  We
// give the user this option as sometimes defining a << operator for
// Foo is not desirable (e.g. the coding style may prevent doing it,
// or there is already a << operator but it doesn't do what the user
// wants).
template <typename T>
void PrintTo(const T& value, ::std::ostream* os) {
  internal::PrintWithFallback(value, os);
}

// The following list of PrintTo() overloads tells
// UniversalPrinter<T>::Print() how to print standard types (built-in
// types, strings, plain arrays, and pointers).

// Overloads for various char types.
GTEST_API_ void PrintTo(unsigned char c, ::std::ostream* os);
GTEST_API_ void PrintTo(signed char c, ::std::ostream* os);
inline void PrintTo(char c, ::std::ostream* os) {
  // When printing a plain char, we always treat it as unsigned.  This
  // way, the output won't be affected by whether the compiler thinks
  // char is signed or not.
  PrintTo(static_cast<unsigned char>(c), os);
}

// Overloads for other simple built-in types.
inline void PrintTo(bool x, ::std::ostream* os) {
  *os << (x ? "true" : "false");
}

// Overload for wchar_t type.
// Prints a wchar_t as a symbol if it is printable or as its internal
// code otherwise and also as its decimal code (except for L'\0').
// The L'\0' char is printed as "L'\\0'". The decimal code is printed
// as signed integer when wchar_t is implemented by the compiler
// as a signed type and is printed as an unsigned integer when wchar_t
// is implemented as an unsigned type.
GTEST_API_ void PrintTo(wchar_t wc, ::std::ostream* os);

GTEST_API_ void PrintTo(char32_t c, ::std::ostream* os);
inline void PrintTo(char16_t c, ::std::ostream* os) {
  PrintTo(ImplicitCast_<char32_t>(c), os);
}
#ifdef __cpp_char8_t
inline void PrintTo(char8_t c, ::std::ostream* os) {
  PrintTo(ImplicitCast_<char32_t>(c), os);
}
#endif

// Overloads for C strings.
GTEST_API_ void PrintTo(const char* s, ::std::ostream* os);
inline void PrintTo(char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const char*>(s), os);
}

// signed/unsigned char is often used for representing binary data, so
// we print pointers to it as void* to be safe.
inline void PrintTo(const signed char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
inline void PrintTo(signed char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
inline void PrintTo(const unsigned char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
inline void PrintTo(unsigned char* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const void*>(s), os);
}
#ifdef __cpp_char8_t
// Overloads for u8 strings.
GTEST_API_ void PrintTo(const char8_t* s, ::std::ostream* os);
inline void PrintTo(char8_t* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const char8_t*>(s), os);
}
#endif
// Overloads for u16 strings.
GTEST_API_ void PrintTo(const char16_t* s, ::std::ostream* os);
inline void PrintTo(char16_t* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const char16_t*>(s), os);
}
// Overloads for u32 strings.
GTEST_API_ void PrintTo(const char32_t* s, ::std::ostream* os);
inline void PrintTo(char32_t* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const char32_t*>(s), os);
}

// MSVC can be configured to define wchar_t as a typedef of unsigned
// short.  It defines _NATIVE_WCHAR_T_DEFINED when wchar_t is a native
// type.  When wchar_t is a typedef, defining an overload for const
// wchar_t* would cause unsigned short* be printed as a wide string,
// possibly causing invalid memory accesses.
#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)
// Overloads for wide C strings
GTEST_API_ void PrintTo(const wchar_t* s, ::std::ostream* os);
inline void PrintTo(wchar_t* s, ::std::ostream* os) {
  PrintTo(ImplicitCast_<const wchar_t*>(s), os);
}
#endif

// Overload for C arrays.  Multi-dimensional arrays are printed
// properly.

// Prints the given number of elements in an array, without printing
// the curly braces.
template <typename T>
void PrintRawArrayTo(const T a[], size_t count, ::std::ostream* os) {
  UniversalPrint(a[0], os);
  for (size_t i = 1; i != count; i++) {
    *os << ", ";
    UniversalPrint(a[i], os);
  }
}

// Overloads for ::std::string.
GTEST_API_ void PrintStringTo(const ::std::string&s, ::std::ostream* os);
inline void PrintTo(const ::std::string& s, ::std::ostream* os) {
  PrintStringTo(s, os);
}

// Overloads for ::std::u8string
#ifdef __cpp_char8_t
GTEST_API_ void PrintU8StringTo(const ::std::u8string& s, ::std::ostream* os);
inline void PrintTo(const ::std::u8string& s, ::std::ostream* os) {
  PrintU8StringTo(s, os);
}
#endif

// Overloads for ::std::u16string
GTEST_API_ void PrintU16StringTo(const ::std::u16string& s, ::std::ostream* os);
inline void PrintTo(const ::std::u16string& s, ::std::ostream* os) {
  PrintU16StringTo(s, os);
}

// Overloads for ::std::u32string
GTEST_API_ void PrintU32StringTo(const ::std::u32string& s, ::std::ostream* os);
inline void PrintTo(const ::std::u32string& s, ::std::ostream* os) {
  PrintU32StringTo(s, os);
}

// Overloads for ::std::wstring.
#if GTEST_HAS_STD_WSTRING
GTEST_API_ void PrintWideStringTo(const ::std::wstring&s, ::std::ostream* os);
inline void PrintTo(const ::std::wstring& s, ::std::ostream* os) {
  PrintWideStringTo(s, os);
}
#endif  // GTEST_HAS_STD_WSTRING

#if GTEST_INTERNAL_HAS_STRING_VIEW
// Overload for internal::StringView.
inline void PrintTo(internal::StringView sp, ::std::ostream* os) {
  PrintTo(::std::string(sp), os);
}
#endif  // GTEST_INTERNAL_HAS_STRING_VIEW

inline void PrintTo(std::nullptr_t, ::std::ostream* os) { *os << "(nullptr)"; }

template <typename T>
void PrintTo(std::reference_wrapper<T> ref, ::std::ostream* os) {
  UniversalPrinter<T&>::Print(ref.get(), os);
}

inline const void* VoidifyPointer(const void* p) { return p; }
inline const void* VoidifyPointer(volatile const void* p) {
  return const_cast<const void*>(p);
}

template <typename T, typename Ptr>
void PrintSmartPointer(const Ptr& ptr, std::ostream* os, char) {
  if (ptr == nullptr) {
    *os << "(nullptr)";
  } else {
    // We can't print the value. Just print the pointer..
    *os << "(" << (VoidifyPointer)(ptr.get()) << ")";
  }
}
template <typename T, typename Ptr,
          typename = typename std::enable_if<!std::is_void<T>::value &&
                                             !std::is_array<T>::value>::type>
void PrintSmartPointer(const Ptr& ptr, std::ostream* os, int) {
  if (ptr == nullptr) {
    *os << "(nullptr)";
  } else {
    *os << "(ptr = " << (VoidifyPointer)(ptr.get()) << ", value = ";
    UniversalPrinter<T>::Print(*ptr, os);
    *os << ")";
  }
}

template <typename T, typename D>
void PrintTo(const std::unique_ptr<T, D>& ptr, std::ostream* os) {
  (PrintSmartPointer<T>)(ptr, os, 0);
}

template <typename T>
void PrintTo(const std::shared_ptr<T>& ptr, std::ostream* os) {
  (PrintSmartPointer<T>)(ptr, os, 0);
}

// Helper function for printing a tuple.  T must be instantiated with
// a tuple type.
template <typename T>
void PrintTupleTo(const T&, std::integral_constant<size_t, 0>,
                  ::std::ostream*) {}

template <typename T, size_t I>
void PrintTupleTo(const T& t, std::integral_constant<size_t, I>,
                  ::std::ostream* os) {
  PrintTupleTo(t, std::integral_constant<size_t, I - 1>(), os);
  GTEST_INTENTIONAL_CONST_COND_PUSH_()
  if (I > 1) {
    GTEST_INTENTIONAL_CONST_COND_POP_()
    *os << ", ";
  }
  UniversalPrinter<typename std::tuple_element<I - 1, T>::type>::Print(
      std::get<I - 1>(t), os);
}

template <typename... Types>
void PrintTo(const ::std::tuple<Types...>& t, ::std::ostream* os) {
  *os << "(";
  PrintTupleTo(t, std::integral_constant<size_t, sizeof...(Types)>(), os);
  *os << ")";
}

// Overload for std::pair.
template <typename T1, typename T2>
void PrintTo(const ::std::pair<T1, T2>& value, ::std::ostream* os) {
  *os << '(';
  // We cannot use UniversalPrint(value.first, os) here, as T1 may be
  // a reference type.  The same for printing value.second.
  UniversalPrinter<T1>::Print(value.first, os);
  *os << ", ";
  UniversalPrinter<T2>::Print(value.second, os);
  *os << ')';
}

// Implements printing a non-reference type T by letting the compiler
// pick the right overload of PrintTo() for T.
template <typename T>
class UniversalPrinter {
 public:
  // MSVC warns about adding const to a function type, so we want to
  // disable the warning.
  GTEST_DISABLE_MSC_WARNINGS_PUSH_(4180)

  // Note: we deliberately don't call this PrintTo(), as that name
  // conflicts with ::testing::internal::PrintTo in the body of the
  // function.
  static void Print(const T& value, ::std::ostream* os) {
    // By default, ::testing::internal::PrintTo() is used for printing
    // the value.
    //
    // Thanks to Koenig look-up, if T is a class and has its own
    // PrintTo() function defined in its namespace, that function will
    // be visible here.  Since it is more specific than the generic ones
    // in ::testing::internal, it will be picked by the compiler in the
    // following statement - exactly what we want.
    PrintTo(value, os);
  }

  GTEST_DISABLE_MSC_WARNINGS_POP_()
};

// Remove any const-qualifiers before passing a type to UniversalPrinter.
template <typename T>
class UniversalPrinter<const T> : public UniversalPrinter<T> {};

#if GTEST_INTERNAL_HAS_ANY

// Printer for std::any / absl::any

template <>
class UniversalPrinter<Any> {
 public:
  static void Print(const Any& value, ::std::ostream* os) {
    if (value.has_value()) {
      *os << "value of type " << GetTypeName(value);
    } else {
      *os << "no value";
    }
  }

 private:
  static std::string GetTypeName(const Any& value) {
#if GTEST_HAS_RTTI
    return internal::GetTypeName(value.type());
#else
    static_cast<void>(value);  // possibly unused
    return "<unknown_type>";
#endif  // GTEST_HAS_RTTI
  }
};

#endif  // GTEST_INTERNAL_HAS_ANY

#if GTEST_INTERNAL_HAS_OPTIONAL

// Printer for std::optional / absl::optional

template <typename T>
class UniversalPrinter<Optional<T>> {
 public:
  static void Print(const Optional<T>& value, ::std::ostream* os) {
    *os << '(';
    if (!value) {
      *os << "nullopt";
    } else {
      UniversalPrint(*value, os);
    }
    *os << ')';
  }
};

#endif  // GTEST_INTERNAL_HAS_OPTIONAL

#if GTEST_INTERNAL_HAS_VARIANT

// Printer for std::variant / absl::variant

template <typename... T>
class UniversalPrinter<Variant<T...>> {
 public:
  static void Print(const Variant<T...>& value, ::std::ostream* os) {
    *os << '(';
#if GTEST_HAS_ABSL
    absl::visit(Visitor{os, value.index()}, value);
#else
    std::visit(Visitor{os, value.index()}, value);
#endif  // GTEST_HAS_ABSL
    *os << ')';
  }

 private:
  struct Visitor {
    template <typename U>
    void operator()(const U& u) const {
      *os << "'" << GetTypeName<U>() << "(index = " << index
          << ")' with value ";
      UniversalPrint(u, os);
    }
    ::std::ostream* os;
    std::size_t index;
  };
};

#endif  // GTEST_INTERNAL_HAS_VARIANT

// UniversalPrintArray(begin, len, os) prints an array of 'len'
// elements, starting at address 'begin'.
template <typename T>
void UniversalPrintArray(const T* begin, size_t len, ::std::ostream* os) {
  if (len == 0) {
    *os << "{}";
  } else {
    *os << "{ ";
    const size_t kThreshold = 18;
    const size_t kChunkSize = 8;
    // If the array has more than kThreshold elements, we'll have to
    // omit some details by printing only the first and the last
    // kChunkSize elements.
    if (len <= kThreshold) {
      PrintRawArrayTo(begin, len, os);
    } else {
      PrintRawArrayTo(begin, kChunkSize, os);
      *os << ", ..., ";
      PrintRawArrayTo(begin + len - kChunkSize, kChunkSize, os);
    }
    *os << " }";
  }
}
// This overload prints a (const) char array compactly.
GTEST_API_ void UniversalPrintArray(
    const char* begin, size_t len, ::std::ostream* os);

#ifdef __cpp_char8_t
// This overload prints a (const) char8_t array compactly.
GTEST_API_ void UniversalPrintArray(const char8_t* begin, size_t len,
                                    ::std::ostream* os);
#endif

// This overload prints a (const) char16_t array compactly.
GTEST_API_ void UniversalPrintArray(const char16_t* begin, size_t len,
                                    ::std::ostream* os);

// This overload prints a (const) char32_t array compactly.
GTEST_API_ void UniversalPrintArray(const char32_t* begin, size_t len,
                                    ::std::ostream* os);

// This overload prints a (const) wchar_t array compactly.
GTEST_API_ void UniversalPrintArray(
    const wchar_t* begin, size_t len, ::std::ostream* os);

// Implements printing an array type T[N].
template <typename T, size_t N>
class UniversalPrinter<T[N]> {
 public:
  // Prints the given array, omitting some elements when there are too
  // many.
  static void Print(const T (&a)[N], ::std::ostream* os) {
    UniversalPrintArray(a, N, os);
  }
};

// Implements printing a reference type T&.
template <typename T>
class UniversalPrinter<T&> {
 public:
  // MSVC warns about adding const to a function type, so we want to
  // disable the warning.
  GTEST_DISABLE_MSC_WARNINGS_PUSH_(4180)

  static void Print(const T& value, ::std::ostream* os) {
    // Prints the address of the value.  We use reinterpret_cast here
    // as static_cast doesn't compile when T is a function type.
    *os << "@" << reinterpret_cast<const void*>(&value) << " ";

    // Then prints the value itself.
    UniversalPrint(value, os);
  }

  GTEST_DISABLE_MSC_WARNINGS_POP_()
};

// Prints a value tersely: for a reference type, the referenced value
// (but not the address) is printed; for a (const) char pointer, the
// NUL-terminated string (but not the pointer) is printed.

template <typename T>
class UniversalTersePrinter {
 public:
  static void Print(const T& value, ::std::ostream* os) {
    UniversalPrint(value, os);
  }
};
template <typename T>
class UniversalTersePrinter<T&> {
 public:
  static void Print(const T& value, ::std::ostream* os) {
    UniversalPrint(value, os);
  }
};
template <typename T, size_t N>
class UniversalTersePrinter<T[N]> {
 public:
  static void Print(const T (&value)[N], ::std::ostream* os) {
    UniversalPrinter<T[N]>::Print(value, os);
  }
};
template <>
class UniversalTersePrinter<const char*> {
 public:
  static void Print(const char* str, ::std::ostream* os) {
    if (str == nullptr) {
      *os << "NULL";
    } else {
      UniversalPrint(std::string(str), os);
    }
  }
};
template <>
class UniversalTersePrinter<char*> : public UniversalTersePrinter<const char*> {
};

#ifdef __cpp_char8_t
template <>
class UniversalTersePrinter<const char8_t*> {
 public:
  static void Print(const char8_t* str, ::std::ostream* os) {
    if (str == nullptr) {
      *os << "NULL";
    } else {
      UniversalPrint(::std::u8string(str), os);
    }
  }
};
template <>
class UniversalTersePrinter<char8_t*>
    : public UniversalTersePrinter<const char8_t*> {};
#endif

template <>
class UniversalTersePrinter<const char16_t*> {
 public:
  static void Print(const char16_t* str, ::std::ostream* os) {
    if (str == nullptr) {
      *os << "NULL";
    } else {
      UniversalPrint(::std::u16string(str), os);
    }
  }
};
template <>
class UniversalTersePrinter<char16_t*>
    : public UniversalTersePrinter<const char16_t*> {};

template <>
class UniversalTersePrinter<const char32_t*> {
 public:
  static void Print(const char32_t* str, ::std::ostream* os) {
    if (str == nullptr) {
      *os << "NULL";
    } else {
      UniversalPrint(::std::u32string(str), os);
    }
  }
};
template <>
class UniversalTersePrinter<char32_t*>
    : public UniversalTersePrinter<const char32_t*> {};

#if GTEST_HAS_STD_WSTRING
template <>
class UniversalTersePrinter<const wchar_t*> {
 public:
  static void Print(const wchar_t* str, ::std::ostream* os) {
    if (str == nullptr) {
      *os << "NULL";
    } else {
      UniversalPrint(::std::wstring(str), os);
    }
  }
};
#endif

template <>
class UniversalTersePrinter<wchar_t*> {
 public:
  static void Print(wchar_t* str, ::std::ostream* os) {
    UniversalTersePrinter<const wchar_t*>::Print(str, os);
  }
};

template <typename T>
void UniversalTersePrint(const T& value, ::std::ostream* os) {
  UniversalTersePrinter<T>::Print(value, os);
}

// Prints a value using the type inferred by the compiler.  The
// difference between this and UniversalTersePrint() is that for a
// (const) char pointer, this prints both the pointer and the
// NUL-terminated string.
template <typename T>
void UniversalPrint(const T& value, ::std::ostream* os) {
  // A workarond for the bug in VC++ 7.1 that prevents us from instantiating
  // UniversalPrinter with T directly.
  typedef T T1;
  UniversalPrinter<T1>::Print(value, os);
}

typedef ::std::vector< ::std::string> Strings;

  // Tersely prints the first N fields of a tuple to a string vector,
  // one element for each field.
template <typename Tuple>
void TersePrintPrefixToStrings(const Tuple&, std::integral_constant<size_t, 0>,
                               Strings*) {}
template <typename Tuple, size_t I>
void TersePrintPrefixToStrings(const Tuple& t,
                               std::integral_constant<size_t, I>,
                               Strings* strings) {
  TersePrintPrefixToStrings(t, std::integral_constant<size_t, I - 1>(),
                            strings);
  ::std::stringstream ss;
  UniversalTersePrint(std::get<I - 1>(t), &ss);
  strings->push_back(ss.str());
}

// Prints the fields of a tuple tersely to a string vector, one
// element for each field.  See the comment before
// UniversalTersePrint() for how we define "tersely".
template <typename Tuple>
Strings UniversalTersePrintTupleFieldsToStrings(const Tuple& value) {
  Strings result;
  TersePrintPrefixToStrings(
      value, std::integral_constant<size_t, std::tuple_size<Tuple>::value>(),
      &result);
  return result;
}

}  // namespace internal

template <typename T>
::std::string PrintToString(const T& value) {
  ::std::stringstream ss;
  internal::UniversalTersePrinter<T>::Print(value, &ss);
  return ss.str();
}

}  // namespace testing

// Include any custom printer added by the local installation.
// We must include this header at the end to make sure it can use the
// declarations from this file.
#include "gtest/internal/custom/gtest-printers.h"

#endif  // GOOGLETEST_INCLUDE_GTEST_GTEST_PRINTERS_H_
