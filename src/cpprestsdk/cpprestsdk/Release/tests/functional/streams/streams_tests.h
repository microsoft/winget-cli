/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * streams_tests.h
 *
 * Common routines for streams tests.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#include <system_error>
#include <unittestpp.h>

namespace tests
{
namespace functional
{
namespace streams
{
template<typename CharType>
void test_stream_length(concurrency::streams::basic_istream<CharType> istr, size_t length)
{
    using namespace concurrency::streams;

    auto curr = istr.tell();
    auto t1 = (curr != static_cast<typename basic_istream<CharType>::pos_type>(basic_istream<CharType>::traits::eof()));
    VERIFY_IS_TRUE(t1);

    auto end = istr.seek(0, std::ios_base::end);
    VERIFY_IS_TRUE(end !=
                   static_cast<typename basic_istream<CharType>::pos_type>(basic_istream<CharType>::traits::eof()));

    auto len = end - curr;

    VERIFY_ARE_EQUAL(len, length);

    {
        auto curr2 = istr.tell();
        VERIFY_IS_TRUE(curr !=
                       static_cast<typename basic_istream<CharType>::pos_type>(basic_istream<CharType>::traits::eof()));

        auto end2 = istr.seek(0, std::ios_base::end);
        VERIFY_IS_TRUE(end !=
                       static_cast<typename basic_istream<CharType>::pos_type>(basic_istream<CharType>::traits::eof()));

        auto len2 = end2 - curr2;

        VERIFY_ARE_EQUAL(len2, 0);
    }

    auto newpos = istr.seek(curr);
    VERIFY_IS_TRUE(newpos !=
                   static_cast<typename basic_istream<CharType>::pos_type>(basic_istream<CharType>::traits::eof()));

    VERIFY_ARE_EQUAL(curr, newpos);
}

// Helper function to verify std::system_error is thrown with correct error code
#define VERIFY_THROWS_SYSTEM_ERROR(__expression, __code)                                                               \
    UNITTEST_MULTILINE_MACRO_BEGIN                                                                                     \
    try                                                                                                                \
    {                                                                                                                  \
        __expression;                                                                                                  \
        VERIFY_IS_TRUE(false, "Expected std::system_error not thrown");                                                \
    }                                                                                                                  \
    catch (const std::system_error& _exc)                                                                              \
    {                                                                                                                  \
        VERIFY_IS_TRUE(std::string(_exc.what()).size() > 0);                                                           \
        /* The reason we can't directly compare with the given std::errc code is because*/                             \
        /* on Windows the STL implementation of error categories are NOT unique across*/                               \
        /* dll boundaries.*/                                                                                           \
        const std::error_condition _condFound = _exc.code().default_error_condition();                                 \
        VERIFY_ARE_EQUAL(static_cast<int>(__code), _condFound.value());                                                \
    }                                                                                                                  \
    catch (...)                                                                                                        \
    {                                                                                                                  \
        VERIFY_IS_TRUE(false, "Exception other than std::system_error thrown");                                        \
    }                                                                                                                  \
    UNITTEST_MULTILINE_MACRO_END

} // namespace streams
} // namespace functional
} // namespace tests
