/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * nonce_generator_tests.cpp
 *
 * Tests for nonce_generator class.
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
SUITE(nonce_generator_tests)
{
    TEST(nonce_generator_set_length)
    {
        utility::nonce_generator gen;
        VERIFY_ARE_EQUAL(utility::nonce_generator::default_length, gen.generate().length());

        gen.set_length(1);
        VERIFY_ARE_EQUAL(1, gen.generate().length());

        gen.set_length(0);
        VERIFY_ARE_EQUAL(0, gen.generate().length());

        gen.set_length(500);
        VERIFY_ARE_EQUAL(500, gen.generate().length());
    }

    TEST(nonce_generator_unique_strings)
    {
        // Generate 100 nonces and check each is unique.
        std::vector<utility::string_t> nonces(100);
        utility::nonce_generator gen;
        for (auto&& v : nonces)
        {
            v = gen.generate();
        }
        for (auto v : nonces)
        {
            VERIFY_ARE_EQUAL(1, std::count(nonces.begin(), nonces.end(), v));
        }
    }

} // SUITE(nonce_generator_tests)

} // namespace utils_tests
} // namespace functional
} // namespace tests
