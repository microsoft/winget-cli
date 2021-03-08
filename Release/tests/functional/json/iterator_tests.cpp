/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * iterator_tests.cpp
 *
 * Tests iterating over JSON values
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/json.h"
#include "unittestpp.h"
#include <algorithm>

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace json_tests
{
SUITE(iterator_tests)
{
    void validate_array_and_object_throw(json::value value)
    {
        VERIFY_THROWS(value.as_array(), web::json::json_exception);
        VERIFY_THROWS(value.as_object(), web::json::json_exception);
    }

    TEST(non_composites_member_preincrement)
    {
        validate_array_and_object_throw(json::value::null());
        validate_array_and_object_throw(json::value::number(17));
        validate_array_and_object_throw(json::value::boolean(true));
        validate_array_and_object_throw(json::value::string(U("Hello!")));
    }

    TEST(objects_constructed)
    {
        json::value val1;
        val1[U("a")] = 44;
        val1[U("b")] = json::value(true);
        val1[U("c")] = json::value(false);

        VERIFY_ARE_EQUAL(3, val1.size());

        size_t count = 0;
        for (auto iter = std::begin(val1.as_object()); iter != std::end(val1.as_object()); ++iter)
        {
            auto key = iter->first;
            auto& value = iter->second;
            switch (count)
            {
                case 0:
                    VERIFY_ARE_EQUAL(U("a"), key);
                    VERIFY_IS_TRUE(value.is_number());
                    break;
                case 1:
                    VERIFY_ARE_EQUAL(U("b"), key);
                    VERIFY_IS_TRUE(value.is_boolean());
                    break;
                case 2:
                    VERIFY_ARE_EQUAL(U("c"), key);
                    VERIFY_IS_TRUE(value.is_boolean());
                    break;
            }
            count++;
        }
        VERIFY_ARE_EQUAL(3, count);
    }

    TEST(objects_parsed)
    {
        json::value val1 = json::value::parse(U("{\"a\": 44, \"b\": true, \"c\": false}"));

        VERIFY_ARE_EQUAL(3, val1.size());

        size_t count = 0;
        for (auto iter = std::begin(val1.as_object()); iter != std::end(val1.as_object()); ++iter)
        {
            auto key = iter->first;
            auto& value = iter->second;
            switch (count)
            {
                default: VERIFY_IS_TRUE(value.is_null()); break;
                case 0:
                    VERIFY_ARE_EQUAL(U("a"), key);
                    VERIFY_IS_TRUE(value.is_number());
                    VERIFY_ARE_EQUAL(44, value.as_integer());
                    break;
                case 1:
                    VERIFY_ARE_EQUAL(U("b"), key);
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_TRUE(value.as_bool());
                    break;
                case 2:
                    VERIFY_ARE_EQUAL(U("c"), key);
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_FALSE(value.as_bool());
                    break;
            }
            count++;
        }
        VERIFY_ARE_EQUAL(3, count);
    }

    TEST(objects_reverse)
    {
        json::value val1 = json::value::parse(U("{\"a\": 44, \"b\": true, \"c\": false}"));

        VERIFY_ARE_EQUAL(3, val1.size());
        VERIFY_ARE_EQUAL(3, val1.as_object().size());

        size_t count = 0;
        for (auto iter = val1.as_object().rbegin(); iter != val1.as_object().rend(); ++iter)
        {
            auto key = iter->first;
            auto& value = iter->second;
            switch (count)
            {
                case 2:
                    VERIFY_ARE_EQUAL(U("a"), key);
                    VERIFY_IS_TRUE(value.is_number());
                    VERIFY_ARE_EQUAL(44, value.as_integer());
                    break;
                case 1:
                    VERIFY_ARE_EQUAL(U("b"), key);
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_TRUE(value.as_bool());
                    break;
                case 0:
                    VERIFY_ARE_EQUAL(U("c"), key);
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_FALSE(value.as_bool());
                    break;
            }
            count++;
        }
        VERIFY_ARE_EQUAL(3, count);
    }

    TEST(arrays_constructed)
    {
        json::value val1;
        val1[0] = 44;
        val1[2] = json::value(true);
        val1[5] = json::value(true);

        VERIFY_ARE_EQUAL(6, val1.size());

        size_t count = 0;
        for (auto value : val1.as_array())
        {
            switch (count)
            {
                case 0:
                    VERIFY_IS_TRUE(value.is_number());
                    VERIFY_ARE_EQUAL(44, value.as_integer());
                    break;
                case 2:
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_TRUE(value.as_bool());
                    break;
                case 5:
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_TRUE(value.as_bool());
                    break;
            }
            count++;
        }
        VERIFY_ARE_EQUAL(6, count);
    }

    TEST(arrays_parsed)
    {
        json::value val1 = json::value::parse(U("[44, true, false]"));

        VERIFY_ARE_EQUAL(3, val1.size());

        size_t count = 0;
        for (auto& value : val1.as_array())
        {
            switch (count)
            {
                case 0:
                    VERIFY_IS_TRUE(value.is_number());
                    VERIFY_ARE_EQUAL(44, value.as_integer());
                    break;
                case 1:
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_TRUE(value.as_bool());
                    break;
                case 2:
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_FALSE(value.as_bool());
                    break;
            }
            count++;
        }
        VERIFY_ARE_EQUAL(3, count);
    }

    TEST(arrays_reversed)
    {
        json::value val1 = json::value::parse(U("[44, true, false]"));

        VERIFY_ARE_EQUAL(3, val1.size());

        size_t count = 0;
        for (auto iter = val1.as_array().rbegin(); iter != val1.as_array().rend(); ++iter)
        {
            auto value = *iter;
            switch (count)
            {
                case 2:
                    VERIFY_IS_TRUE(value.is_number());
                    VERIFY_ARE_EQUAL(44, value.as_integer());
                    break;
                case 1:
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_TRUE(value.as_bool());
                    break;
                case 0:
                    VERIFY_IS_TRUE(value.is_boolean());
                    VERIFY_IS_FALSE(value.as_bool());
                    break;
            }
            count++;
        }
        VERIFY_ARE_EQUAL(3, count);
    }

    TEST(comparison)
    {
        json::value val1;
        val1[U("a")] = 44;
        val1[U("b")] = json::value(true);
        val1[U("c")] = json::value(false);

        auto first = std::begin(val1.as_object());
        auto f = first;
        auto f_1 = first++;
        auto f_2 = ++first;

        VERIFY_ARE_EQUAL(f, f_1);
        VERIFY_ARE_NOT_EQUAL(f_1, f_2);
    }

    TEST(std_algorithms)
    {
        {
            // for_each
            size_t count = 0;
            json::value v_array = json::value::parse(U("[44, true, false]"));
            std::for_each(std::begin(v_array.as_array()),
                          std::end(v_array.as_array()),
                          [&](json::array::iterator::value_type) { count++; });
            VERIFY_ARE_EQUAL(3, count);
        }
        {
            // find_if
            json::value v_array = json::value::parse(U("[44, true, false]"));
            auto _where = std::find_if(std::begin(v_array.as_array()),
                                       std::end(v_array.as_array()),
                                       [&](json::array::iterator::value_type value) { return value.is_boolean(); });

            VERIFY_ARE_NOT_EQUAL(_where, std::end(v_array.as_array()));

            VERIFY_ARE_EQUAL(_where->as_bool(), true);
        }
        {
            // copy_if
            json::value v_array = json::value::parse(U("[44, true, false]"));
            std::vector<json::array::iterator::value_type> v_target(v_array.size());
            auto _where = std::copy_if(std::begin(v_array.as_array()),
                                       std::end(v_array.as_array()),
                                       std::begin(v_target),
                                       [&](json::array::iterator::value_type value) { return value.is_boolean(); });
            VERIFY_ARE_EQUAL(2, _where - std::begin(v_target));
            VERIFY_IS_FALSE(v_array.as_array().begin()[1].is_number());
        }
        {
            // transform
            json::value v_array = json::value::parse(U("[44, true, false]"));
            std::vector<json::value> v_target(v_array.size());
            std::transform(std::begin(v_array.as_array()),
                           std::end(v_array.as_array()),
                           std::begin(v_target),
                           [&](json::array::iterator::value_type) -> json::value { return json::value::number(17); });

            VERIFY_ARE_EQUAL(3, v_target.size());

            for (auto iter = std::begin(v_target); iter != std::end(v_target); ++iter)
            {
                VERIFY_IS_FALSE(iter->is_null());
            }
        }
    }
}

} // namespace json_tests
} // namespace functional
} // namespace tests
