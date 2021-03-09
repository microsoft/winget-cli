/***
 * This file is based on or incorporates material from the UnitTest++ r30 open source project.
 * Microsoft is not the original author of this code but has modified it and is licensing the code under
 * the MIT License. Microsoft reserves all other rights not expressly granted under the MIT License,
 * whether by implication, estoppel or otherwise.
 *
 * UnitTest++ r30
 *
 * Copyright (c) 2006 Noel Llopis and Charles Nicholson
 * Portions Copyright (c) Microsoft Corporation
 *
 * All Rights Reserved.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ***/

#ifndef UNITTEST_CHECKS_H
#define UNITTEST_CHECKS_H

#include "MemoryOutStream.h"
#include "TestResults.h"
#include "config.h"
#include <cstring>
#include <memory>
#include <string>

#ifndef _WIN32
#include <boost/locale/encoding_utf.hpp>
#endif

namespace UnitTest
{
namespace details
{
inline std::string utf16_to_utf8(const std::basic_string<utf16char>& w)
{
#ifdef _WIN32
    std::string result;
    size_t size;
    wcstombs_s(&size, nullptr, 0, (const wchar_t*)w.c_str(), w.size());
    result.resize(size);
    // integr0808: added cast
    wcstombs_s(&size, &result[0], size, (const wchar_t*)w.c_str(), w.size());
    return result;
#else
    return boost::locale::conv::utf_to_utf<char, utf16char>(w, boost::locale::conv::stop);
#endif
}

typedef char yes;
typedef char (&no)[2];

struct anyx
{
    template<typename T>
    anyx(const T&);
};
no operator<<(const anyx&, const anyx&);

template<typename T>
yes check(T const&);
no check(no);

template<typename StreamType, typename T1, typename T2>
struct support_stream_write
{
    static StreamType& stream;
    static T1& x;
    static T2& y;
    static const bool value =
        (sizeof(check(stream << x)) == sizeof(yes)) && (sizeof(check(stream << y)) == sizeof(yes));
};

template<typename T1, typename T2>
inline std::string BuildFailureStringWithStream(const char* expectedStr,
                                                const char* actualStr,
                                                const T1& expected,
                                                const T2& actual)
{
    UnitTest::MemoryOutStream stream;
    stream << " where " << expectedStr << "=" << expected << " and " << actualStr << "=" << actual;
    return stream.GetText();
}

template<typename T1, typename T2, bool UseStreams>
struct BuildFailureStringImpl
{
    std::string BuildString(const char*, const char*, const T1&, const T2&)
    {
        // Don't do anything since operator<< isn't supported.
        return std::string{};
    }
};

template<typename T1, typename T2>
struct BuildFailureStringImpl<T1, T2, true>
{
    std::string BuildString(const char* expectedStr, const char* actualStr, const T1& expected, const T2& actual)
    {
        return BuildFailureStringWithStream(expectedStr, actualStr, expected, actual);
    }
};

template<typename T1, typename T2>
inline std::string BuildFailureString(const char* expectedStr,
                                      const char* actualStr,
                                      const T1& expected,
                                      const T2& actual)
{
    return BuildFailureStringImpl<T1, T2, support_stream_write<UnitTest::MemoryOutStream, T1, T2>::value>().BuildString(
        expectedStr, actualStr, expected, actual);
}
inline std::string BuildFailureString(const char* expectedStr,
                                      const char* actualStr,
                                      const std::basic_string<utf16char>& expected,
                                      const std::basic_string<utf16char>& actual)
{
    return BuildFailureStringWithStream(expectedStr, actualStr, utf16_to_utf8(expected), utf16_to_utf8(actual));
}
inline std::string BuildFailureString(const char* expectedStr,
                                      const char* actualStr,
                                      const utf16char* expected,
                                      const utf16char* actual)
{
    return BuildFailureStringWithStream(expectedStr, actualStr, utf16_to_utf8(expected), utf16_to_utf8(actual));
}
inline std::string BuildFailureString(const char* expectedStr,
                                      const char* actualStr,
                                      const std::basic_string<utf16char>& expected,
                                      const utf16char* actual)
{
    return BuildFailureStringWithStream(expectedStr, actualStr, utf16_to_utf8(expected), utf16_to_utf8(actual));
}
inline std::string BuildFailureString(const char* expectedStr,
                                      const char* actualStr,
                                      const utf16char* expected,
                                      const std::basic_string<utf16char>& actual)
{
    return BuildFailureStringWithStream(expectedStr, actualStr, utf16_to_utf8(expected), utf16_to_utf8(actual));
}
} // namespace details

template<typename Value>
bool Check(Value const value)
{
    return !!value; // doing double negative to avoid silly VS warnings
}

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4389)
#endif
template<typename Expected, typename Actual>
bool CheckEqualImpl(const Expected& expected, const Actual& actual)
{
    return !(expected == actual);
}
#ifdef _WIN32
#pragma warning(pop)
#endif

inline bool CheckEqualImpl(const char* expected, const char* actual) { return !(std::strcmp(expected, actual) == 0); }
inline bool CheckEqualImpl(char* expected, const char* actual) { return !(std::strcmp(expected, actual) == 0); }
inline bool CheckEqualImpl(const char* expected, char* actual) { return !(std::strcmp(expected, actual) == 0); }
inline bool CheckEqualImpl(char* expected, char* actual) { return !(std::strcmp(expected, actual) == 0); }
inline bool CheckEqualImpl(const wchar_t* expected, const wchar_t* actual) { return !(wcscmp(expected, actual) == 0); }
inline bool CheckEqualImpl(wchar_t* expected, const wchar_t* actual) { return !(wcscmp(expected, actual) == 0); }
inline bool CheckEqualImpl(const wchar_t* expected, wchar_t* actual) { return !(wcscmp(expected, actual) == 0); }
inline bool CheckEqualImpl(wchar_t* expected, wchar_t* actual) { return !(wcscmp(expected, actual) == 0); }

template<typename Expected, typename Actual>
void CheckEqual(TestResults& results,
                const char* expectedStr,
                const char* actualStr,
                const Expected& expected,
                const Actual& actual,
                TestDetails const& details,
                const char* msg = nullptr)
{
    if (CheckEqualImpl(expected, actual))
    {
        UnitTest::MemoryOutStream stream;
        stream << "CHECK_EQUAL(" << expectedStr << ", " << actualStr << ")";
        stream << details::BuildFailureString(expectedStr, actualStr, expected, actual) << std::endl;
        if (msg != nullptr)
        {
            stream << msg;
        }
        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Expected, typename Actual>
void CheckNotEqual(TestResults& results,
                   const char* expectedStr,
                   const char* actualStr,
                   Expected const& expected,
                   Actual const& actual,
                   TestDetails const& details,
                   const char* msg = nullptr)
{
    if (!CheckEqualImpl(expected, actual))
    {
        UnitTest::MemoryOutStream stream;
        stream << "CHECK_NOT_EQUAL(" << expectedStr << ", " << actualStr << ")";
        stream << details::BuildFailureString(expectedStr, actualStr, expected, actual) << std::endl;
        if (msg != nullptr)
        {
            stream << msg;
        }
        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Actual>
void CheckNull(TestResults& results, const char* actualStr, Actual const& actual, TestDetails const& details)
{
    if (actual)
    {
        UnitTest::MemoryOutStream stream;
        stream << "CHECK_NULL(" << actualStr << ")";
        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Actual>
void CheckNotNull(TestResults& results, const char* actualStr, Actual const& actual, TestDetails const& details)
{
    if (!actual)
    {
        UnitTest::MemoryOutStream stream;
        stream << "CHECK_NOT_NULL(" << actualStr << ")";
        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Expected, typename Actual, typename Tolerance>
bool AreClose(Expected const& expected, Actual const& actual, Tolerance const& tolerance)
{
    return (actual >= (expected - tolerance)) && (actual <= (expected + tolerance));
}

template<typename Expected, typename Actual, typename Tolerance>
void CheckClose(TestResults& results,
                Expected const& expected,
                Actual const& actual,
                Tolerance const& tolerance,
                TestDetails const& details)
{
    if (!AreClose(expected, actual, tolerance))
    {
        UnitTest::MemoryOutStream stream;
        stream << "Expected " << expected << " +/- " << tolerance << " but was " << actual;

        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Expected, typename Actual>
void CheckArrayEqual(
    TestResults& results, Expected const& expected, Actual const& actual, int const count, TestDetails const& details)
{
    bool equal = true;
    for (int i = 0; i < count; ++i)
        equal &= (expected[i] == actual[i]);

    if (!equal)
    {
        UnitTest::MemoryOutStream stream;

        stream << "Expected [ ";

        for (int expectedIndex = 0; expectedIndex < count; ++expectedIndex)
            stream << expected[expectedIndex] << " ";

        stream << "] but was [ ";

        for (int actualIndex = 0; actualIndex < count; ++actualIndex)
            stream << actual[actualIndex] << " ";

        stream << "]";

        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Expected, typename Actual, typename Tolerance>
bool ArrayAreClose(Expected const& expected, Actual const& actual, int const count, Tolerance const& tolerance)
{
    bool equal = true;
    for (int i = 0; i < count; ++i)
        equal &= AreClose(expected[i], actual[i], tolerance);
    return equal;
}

template<typename Expected, typename Actual, typename Tolerance>
void CheckArrayClose(TestResults& results,
                     Expected const& expected,
                     Actual const& actual,
                     int const count,
                     Tolerance const& tolerance,
                     TestDetails const& details)
{
    bool equal = ArrayAreClose(expected, actual, count, tolerance);

    if (!equal)
    {
        UnitTest::MemoryOutStream stream;

        stream << "Expected [ ";
        for (int expectedIndex = 0; expectedIndex < count; ++expectedIndex)
            stream << expected[expectedIndex] << " ";
        stream << "] +/- " << tolerance << " but was [ ";

        for (int actualIndex = 0; actualIndex < count; ++actualIndex)
            stream << actual[actualIndex] << " ";
        stream << "]";

        results.OnTestFailure(details, stream.GetText());
    }
}

template<typename Expected, typename Actual, typename Tolerance>
void CheckArray2DClose(TestResults& results,
                       Expected const& expected,
                       Actual const& actual,
                       int const rows,
                       int const columns,
                       Tolerance const& tolerance,
                       TestDetails const& details)
{
    bool equal = true;
    for (int i = 0; i < rows; ++i)
        equal &= ArrayAreClose(expected[i], actual[i], columns, tolerance);

    if (!equal)
    {
        UnitTest::MemoryOutStream stream;

        stream << "Expected [ ";

        for (int expectedRow = 0; expectedRow < rows; ++expectedRow)
        {
            stream << "[ ";
            for (int expectedColumn = 0; expectedColumn < columns; ++expectedColumn)
                stream << expected[expectedRow][expectedColumn] << " ";
            stream << "] ";
        }

        stream << "] +/- " << tolerance << " but was [ ";

        for (int actualRow = 0; actualRow < rows; ++actualRow)
        {
            stream << "[ ";
            for (int actualColumn = 0; actualColumn < columns; ++actualColumn)
                stream << actual[actualRow][actualColumn] << " ";
            stream << "] ";
        }

        stream << "]";

        results.OnTestFailure(details, stream.GetText());
    }
}

} // namespace UnitTest

#endif
