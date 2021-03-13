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

#ifndef UNITTEST_CHECKMACROS_H
#define UNITTEST_CHECKMACROS_H

#include "AssertException.h"
#include "Checks.h"
#include "CurrentTest.h"
#include "ExceptionMacros.h"
#include "HelperMacros.h"
#include "MemoryOutStream.h"
#include "ReportAssertImpl.h"
#include "TestDetails.h"
#include <stdarg.h>

#ifdef CHECK
#error UnitTest++ redefines CHECK
#endif

#ifdef CHECK_EQUAL
#error UnitTest++ redefines CHECK_EQUAL
#endif

#ifdef CHECK_CLOSE
#error UnitTest++ redefines CHECK_CLOSE
#endif

#ifdef CHECK_ARRAY_EQUAL
#error UnitTest++ redefines CHECK_ARRAY_EQUAL
#endif

#ifdef CHECK_ARRAY_CLOSE
#error UnitTest++ redefines CHECK_ARRAY_CLOSE
#endif

#ifdef CHECK_ARRAY2D_CLOSE
#error UnitTest++ redefines CHECK_ARRAY2D_CLOSE
#endif

#ifdef VERIFY_IS_TRUE
#error UnitTest++ redefines VERIFY_IS_TRUE
#endif

#ifdef VERIFY_IS_FALSE
#error UnitTest++ redefines VERIFY_IS_FALSE
#endif

#ifdef VERIFY_ARE_EQUAL
#error UnitTest++ redefines VERIFY_ARE_EQUAL
#endif

#ifdef VERIFY_ARE_NOT_EQUAL
#error UnitTest++ redefines VERIFY_ARE_NOT_EQUAL
#endif

#ifdef VERIFY_THROWS
#error UnitTest++ redefines VERIFY_THROWS
#endif

#ifdef VERIFY_IS_NOT_NULL
#error UnitTest++ redefines VERIFY_IS_NOT_NULL
#endif

#ifdef VERIFY_IS_NULL
#error UnitTest++ redefines VERIFY_IS_NULL
#endif

#ifdef WIN32
#define VERIFY_IS_TRUE(expression, ...) CHECK_EQUAL(true, expression, __VA_ARGS__)
#define VERIFY_IS_FALSE(expression, ...) CHECK_EQUAL(false, expression, __VA_ARGS__)
#define VERIFY_ARE_NOT_EQUAL(expected, actual, ...) CHECK_NOT_EQUAL(expected, actual, __VA_ARGS__)
#define VERIFY_ARE_EQUAL(expected, actual, ...) CHECK_EQUAL(expected, actual, __VA_ARGS__)
#else
#define VERIFY_IS_TRUE(expression, ...) CHECK_EQUAL(true, expression, ##__VA_ARGS__)
#define VERIFY_IS_FALSE(expression, ...) CHECK_EQUAL(false, expression, ##__VA_ARGS__)
#define VERIFY_ARE_NOT_EQUAL(expected, actual, ...) CHECK_NOT_EQUAL(expected, actual, ##__VA_ARGS__)
#define VERIFY_ARE_EQUAL(expected, actual, ...) CHECK_EQUAL(expected, actual, ##__VA_ARGS__)
#endif

#define VERIFY_NO_THROWS(expression) CHECK_NO_THROW(expression)
#define VERIFY_THROWS(expression, exception) CHECK_THROW(expression, exception)
#define VERIFY_IS_NOT_NULL(expression) CHECK_NOT_NULL(expression)
#define VERIFY_IS_NULL(expression) CHECK_NULL(expression)

#define CHECK(value)                                                                                                   \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    if (!UnitTest::Check(value))                                                                                       \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__), #value);                               \
    UNITTEST_MULTILINE_MACRO_END

#ifdef WIN32

#define CHECK_EQUAL(expected, actual, ...)                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        UnitTest::CheckEqual(*UnitTest::CurrentTest::Results(),                                                        \
                             #expected,                                                                                \
                             #actual,                                                                                  \
                             expected,                                                                                 \
                             actual,                                                                                   \
                             UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                       \
                             __VA_ARGS__);                                                                             \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_NOT_EQUAL(expected, actual, ...)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        UnitTest::CheckNotEqual(*UnitTest::CurrentTest::Results(),                                                     \
                                #expected,                                                                             \
                                #actual,                                                                               \
                                expected,                                                                              \
                                actual,                                                                                \
                                UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                    \
                                __VA_ARGS__);                                                                          \
    UNITTEST_MULTILINE_MACRO_END

#else

#define CHECK_EQUAL(expected, actual, ...)                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            UnitTest::CheckEqual(*UnitTest::CurrentTest::Results(),                                                    \
                                 #expected,                                                                            \
                                 #actual,                                                                              \
                                 expected,                                                                             \
                                 actual,                                                                               \
                                 UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                   \
                                 ##__VA_ARGS__);                                                                       \
        }                                                                                                              \
        catch (const std::exception& ex)                                                                               \
        {                                                                                                              \
            std::cerr << ex.what() << std::endl;                                                                       \
            UnitTest::CurrentTest::Results()->OnTestFailure(                                                           \
                UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                                    \
                "Unhandled exception in CHECK_EQUAL(" #expected ", " #actual ") - details: ");                         \
        }                                                                                                              \
        UT_CATCH_ALL({                                                                                                 \
            UnitTest::CurrentTest::Results()->OnTestFailure(                                                           \
                UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                                    \
                "Unhandled exception in CHECK_EQUAL(" #expected ", " #actual ")");                                     \
        })                                                                                                             \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_NOT_EQUAL(expected, actual, ...)                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            UnitTest::CheckNotEqual(*UnitTest::CurrentTest::Results(),                                                 \
                                    #expected,                                                                         \
                                    #actual,                                                                           \
                                    expected,                                                                          \
                                    actual,                                                                            \
                                    UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                \
                                    ##__VA_ARGS__);                                                                    \
        }                                                                                                              \
        UT_CATCH_ALL({                                                                                                 \
            UnitTest::CurrentTest::Results()->OnTestFailure(                                                           \
                UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                                    \
                "Unhandled exception in CHECK_NOT_EQUAL(" #expected ", " #actual ")");                                 \
        })                                                                                                             \
    UNITTEST_MULTILINE_MACRO_END
#endif

#define CHECK_NULL(expression)                                                                                         \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::CheckNull(*UnitTest::CurrentTest::Results(),                                                             \
                        #expression,                                                                                   \
                        expression,                                                                                    \
                        UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__));                           \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_NOT_NULL(expression)                                                                                     \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::CheckNotNull(*UnitTest::CurrentTest::Results(),                                                          \
                           #expression,                                                                                \
                           expression,                                                                                 \
                           UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__));                        \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_CLOSE(expected, actual, tolerance)                                                                       \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::CheckClose(*UnitTest::CurrentTest::Results(),                                                            \
                         expected,                                                                                     \
                         actual,                                                                                       \
                         tolerance,                                                                                    \
                         UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__));                          \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_ARRAY_EQUAL(expected, actual, count)                                                                     \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::CheckArrayEqual(*UnitTest::CurrentTest::Results(),                                                       \
                              expected,                                                                                \
                              actual,                                                                                  \
                              count,                                                                                   \
                              UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__));                     \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_ARRAY_CLOSE(expected, actual, count, tolerance)                                                          \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::CheckArrayClose(*UnitTest::CurrentTest::Results(),                                                       \
                              expected,                                                                                \
                              actual,                                                                                  \
                              count,                                                                                   \
                              tolerance,                                                                               \
                              UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__));                     \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_ARRAY2D_CLOSE(expected, actual, rows, columns, tolerance)                                                \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::CheckArray2DClose(*UnitTest::CurrentTest::Results(),                                                     \
                                expected,                                                                              \
                                actual,                                                                                \
                                rows,                                                                                  \
                                columns,                                                                               \
                                tolerance,                                                                             \
                                UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__));                   \
    UNITTEST_MULTILINE_MACRO_END

// CHECK_THROW and CHECK_ASSERT only exist when UNITTEST_NO_EXCEPTIONS isn't defined (see config.h)
#ifndef UNITTEST_NO_EXCEPTIONS
#define CHECK_THROW(expression, ExpectedExceptionType)                                                                 \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    bool caught_ = false;                                                                                              \
    try                                                                                                                \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            expression;                                                                                                \
        }                                                                                                              \
        catch (const std::exception& _exc)                                                                             \
        {                                                                                                              \
            std::string _msg(_exc.what());                                                                             \
            VERIFY_IS_TRUE(_msg.size() > 0);                                                                           \
            throw;                                                                                                     \
        }                                                                                                              \
    }                                                                                                                  \
    catch (ExpectedExceptionType const&)                                                                               \
    {                                                                                                                  \
        caught_ = true;                                                                                                \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
    }                                                                                                                  \
    if (!caught_)                                                                                                      \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__),                                        \
            "Expected exception: \"" #ExpectedExceptionType "\" not thrown");                                          \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_NO_THROW(expression)                                                                                     \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    try                                                                                                                \
    {                                                                                                                  \
        expression;                                                                                                    \
    }                                                                                                                  \
    catch (const std::exception& _exc)                                                                                 \
    {                                                                                                                  \
        std::string _msg("(" #expression ") threw exception: ");                                                       \
        _msg.append(_exc.what());                                                                                      \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__), _msg.c_str());                         \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        std::string _msg("(" #expression ") threw exception: <...>");                                                  \
        UnitTest::CurrentTest::Results()->OnTestFailure(                                                               \
            UnitTest::TestDetails(*UnitTest::CurrentTest::Details(), __LINE__), _msg.c_str());                         \
    }                                                                                                                  \
    UNITTEST_MULTILINE_MACRO_END

#define CHECK_ASSERT(expression)                                                                                       \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    UnitTest::Detail::ExpectAssert(true);                                                                              \
    CHECK_THROW(expression, UnitTest::AssertException);                                                                \
    UnitTest::Detail::ExpectAssert(false);                                                                             \
    UNITTEST_MULTILINE_MACRO_END
#endif
#endif
