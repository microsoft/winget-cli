/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * win32_encryption_tests.cpp
 *
 * Tests for win32_encryption class.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

using namespace utility;

namespace tests
{
namespace functional
{
namespace utils_tests
{
#if defined(_WIN32) && _WIN32_WINNT >= _WIN32_WINNT_VISTA && !defined(__cplusplus_winrt)
SUITE(win32_encryption)
{
    TEST(win32_encryption_random_string)
    {
        utility::string_t rndStr = utility::conversions::to_string_t("random string");
        web::details::win32_encryption enc(rndStr);

        VERIFY_ARE_EQUAL(*enc.decrypt(), rndStr);
    }

    TEST(win32_encryption_empty_string)
    {
        utility::string_t emptyStr = utility::conversions::to_string_t("");
        web::details::win32_encryption enc(emptyStr);

        VERIFY_ARE_EQUAL(*enc.decrypt(), emptyStr);
    }

} // SUITE(win32_encryption)

#endif // defined(_WIN32) && _WIN32_WINNT >= _WIN32_WINNT_VISTA && !defined(__cplusplus_winrt)

} // namespace utils_tests
} // namespace functional
} // namespace tests
