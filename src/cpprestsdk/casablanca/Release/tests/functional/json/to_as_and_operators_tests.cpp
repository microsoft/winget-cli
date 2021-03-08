/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests for to_*, as_*, and operators on JSON values.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/json.h"
#include "unittestpp.h"

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace json_tests
{
SUITE(to_as_and_operators_tests)
{
    TEST(to_string)
    {
        utility::stringstream_t stream(utility::stringstream_t::in | utility::stringstream_t::out);

        // null
        json::value n;
        VERIFY_ARE_EQUAL(U("null"), n.serialize());
        n.serialize(stream);
        VERIFY_ARE_EQUAL(U("null"), stream.str());

        // bool - true
        stream.str(U(""));
        json::value b(true);
        VERIFY_ARE_EQUAL(U("true"), b.serialize());
        b.serialize(stream);
        VERIFY_ARE_EQUAL(U("true"), stream.str());

        // bool - false
        stream.str(U(""));
        json::value b2(false);
        VERIFY_ARE_EQUAL(U("false"), b2.serialize());
        b2.serialize(stream);
        VERIFY_ARE_EQUAL(U("false"), stream.str());

        // number - int
        stream.str(U(""));
        json::value num(44);
        VERIFY_ARE_EQUAL(U("44"), num.serialize());
        num.serialize(stream);
        VERIFY_ARE_EQUAL(U("44"), stream.str());

        // number - double
        stream.str(U(""));
        json::value dNum(11.5);
        VERIFY_ARE_EQUAL(U("11.5"), dNum.serialize());
        dNum.serialize(stream);
        VERIFY_ARE_EQUAL(U("11.5"), stream.str());

        // string
        stream.str(U(""));
        json::value string = json::value::string(U("hehehe"));
        VERIFY_ARE_EQUAL(U("\"hehehe\""), string.serialize());
        string.serialize(stream);
        VERIFY_ARE_EQUAL(U("\"hehehe\""), stream.str());

        // object - with values created from parsing
        stream.str(U(""));
        const utility::string_t strValue1(U("{ \"key\" : true }"));
        const utility::string_t strValue2(U("{\"key\":true}"));
        json::value obj1 = json::value::parse(strValue1);
        VERIFY_ARE_EQUAL(strValue2, obj1.serialize());
        json::value obj2 = json::value::parse(strValue2);
        VERIFY_ARE_EQUAL(strValue2, obj2.serialize());
        obj1.serialize(stream);
        VERIFY_ARE_EQUAL(strValue2, stream.str());

        // object - with values added
        stream.str(U(""));
        json::value obj3 = json::value::object();
        obj3[U("key")] = json::value(true);
        VERIFY_ARE_EQUAL(strValue2, obj3.serialize());
        obj3.serialize(stream);
        VERIFY_ARE_EQUAL(strValue2, stream.str());

        // array
        stream.str(U(""));
        json::value arr = json::value::array();
        arr[0] = json::value::string(U("Here"));
        arr[1] = json::value(true);
        VERIFY_ARE_EQUAL(U("[\"Here\",true]"), arr.serialize());
        VERIFY_ARE_EQUAL(U("[\"Here\",true]"), arr.serialize());
        arr.serialize(stream);
        VERIFY_ARE_EQUAL(U("[\"Here\",true]"), stream.str());
    }

    TEST(empty_arrays_objects)
    {
        // array
        auto arr = json::value::parse(U("[   ]"));
        VERIFY_ARE_EQUAL(U("[]"), arr.serialize());

        // object
        auto obj = json::value::parse(U("{   }"));
        VERIFY_ARE_EQUAL(U("{}"), obj.serialize());
    }

    void verify_escaped_chars(const utility::string_t& str1, const utility::string_t& str2)
    {
        json::value j1 = json::value::string(str1);
        VERIFY_ARE_EQUAL(str2, j1.serialize());
    }

    void verify_unescaped_chars(const utility::string_t& str1, const utility::string_t& str2)
    {
        json::value j1 = json::value::string(str1, false);
        VERIFY_ARE_EQUAL(str2, j1.serialize());
    }

    TEST(to_string_escaped_chars)
    {
        verify_escaped_chars(U(" \" "), U("\" \\\" \""));
        verify_escaped_chars(U(" \b "), U("\" \\b \""));
        verify_escaped_chars(U(" \f "), U("\" \\f \""));
        verify_escaped_chars(U(" \n "), U("\" \\n \""));
        verify_escaped_chars(U(" \r "), U("\" \\r \""));
        verify_escaped_chars(U(" \t "), U("\" \\t \""));

        json::value obj = json::value::object();
        obj[U(" \t ")] = json::value::string(U(" \b "));

        json::value arr = json::value::array();
        arr[0] = json::value::string(U(" \f "));

        VERIFY_ARE_EQUAL(U("{\" \\t \":\" \\b \"}"), obj.serialize());
        VERIFY_ARE_EQUAL(U("[\" \\f \"]"), arr.serialize());

        utility::string_t str(U("{\"hello\":\" \\\"here's looking at you kid\\\" \\r \"}"));
        json::value obj2 = json::value::parse(str);

        VERIFY_ARE_EQUAL(str, obj2.serialize());
    }

    TEST(to_string_unescaped_chars)
    {
        verify_unescaped_chars(U(" \" "), U("\" \" \""));
        verify_unescaped_chars(U(" \b "), U("\" \b \""));
        verify_unescaped_chars(U(" \f "), U("\" \f \""));
        verify_unescaped_chars(U(" \n "), U("\" \n \""));
        verify_unescaped_chars(U(" \r "), U("\" \r \""));
        verify_unescaped_chars(U(" \t "), U("\" \t \""));

        json::value obj = json::value::object();
        obj[U(" \t ")] = json::value::string(U(" \b "), false);

        json::value arr = json::value::array();
        arr[0] = json::value::string(U(" \f "), false);

        VERIFY_ARE_EQUAL(U("{\" \\t \":\" \b \"}"), obj.serialize());
        VERIFY_ARE_EQUAL(U("[\" \f \"]"), arr.serialize());
    }

    TEST(as_string)
    {
        json::value b(false);
        VERIFY_THROWS(b.as_string(), json::json_exception);
        VERIFY_THROWS(b.as_string(), json::json_exception);

        utility::string_t data(U("HERE IS A STRING"));
        utility::string_t wdata(data.begin(), data.end());
        json::value str = json::value::string(data);
        VERIFY_ARE_EQUAL(data, str.as_string());
        VERIFY_ARE_EQUAL(wdata, str.as_string());
    }

    TEST(as_copy_constructor)
    {
        auto arr = json::value::array();
        arr[0] = json::value::number(44);
        arr[1] = json::value::string(U("abc"));
        json::array arrCopy = arr.as_array();
        VERIFY_ARE_EQUAL(2, arrCopy.size());
        VERIFY_ARE_EQUAL(2, arr.size());
        VERIFY_ARE_EQUAL(44, arrCopy[0].as_integer());
        VERIFY_ARE_EQUAL(U("abc"), arrCopy[1].as_string());
        VERIFY_ARE_EQUAL(44, arr[0].as_integer());
        VERIFY_ARE_EQUAL(U("abc"), arr[1].as_string());

        auto obj = json::value::object();
        obj[U("abc")] = json::value::number(123);
        json::object objCopy = obj.as_object();
        VERIFY_ARE_EQUAL(1, objCopy.size());
        VERIFY_ARE_EQUAL(1, obj.size());
        VERIFY_ARE_EQUAL(123, objCopy[U("abc")].as_integer());
        VERIFY_ARE_EQUAL(123, obj[U("abc")].as_integer());

        auto num = json::value::number(44);
        json::number numCopy = num.as_number();
        VERIFY_ARE_EQUAL(44, num.as_integer());
        VERIFY_ARE_EQUAL(44, numCopy.to_int32());
    }

    TEST(as_bool_as_double_as_string)
    {
        utility::stringstream_t ss1;
        ss1 << U("17");
        json::value v1 = json::value::parse(ss1);

        utility::stringstream_t ss2;
        ss2 << U("3.1415");
        json::value v2 = json::value::parse(ss2);

        utility::stringstream_t ss3;
        ss3 << U("true");
        json::value v3 = json::value::parse(ss3);

        utility::stringstream_t ss4;
        ss4 << U("\"Hello!\"");
        json::value v4 = json::value::parse(ss4);

        utility::stringstream_t ss8;
        ss8 << U("{ \"a\" : 10, \"b\" : 4711.17, \"c\" : false }");
        json::value v8 = json::value::parse(ss8);

        utility::stringstream_t ss9;
        ss9 << U("[1,2,3,true]");
        json::value v9 = json::value::parse(ss9);

        VERIFY_ARE_EQUAL(v1.as_double(), 17);
        VERIFY_ARE_EQUAL(v2.as_double(), 3.1415);
        VERIFY_IS_TRUE(v3.as_bool());
        VERIFY_ARE_EQUAL(v4.as_string(), U("Hello!"));
        VERIFY_ARE_EQUAL(v4.as_string(), U("Hello!"));

        VERIFY_ARE_EQUAL(v8[U("a")].as_double(), 10);
        VERIFY_ARE_EQUAL(v8[U("b")].as_double(), 4711.17);
        VERIFY_ARE_EQUAL(v8[U("a")].as_integer(), 10);
        VERIFY_IS_FALSE(v8[U("c")].as_bool());

        VERIFY_ARE_EQUAL(v9[0].as_double(), 1);
        VERIFY_ARE_EQUAL(v9[1].as_double(), 2);
        VERIFY_ARE_EQUAL(v9[2].as_double(), 3);
        VERIFY_IS_TRUE(v9[3].as_bool());
    }

    TEST(to_stream_operator)
    {
        utility::string_t str(U("\"JSON STRING\""));
        json::value value = json::value::parse(str);
        utility::stringstream_t stream;
        stream << value;
        VERIFY_ARE_EQUAL(str, stream.str());
    }

    TEST(from_stream_operator)
    {
        utility::string_t str(U("\"JSON STRING!\""));
        utility::stringstream_t stream;
        stream << str;
        json::value value;
        stream >> value;
        VERIFY_IS_TRUE(value.is_string());
        VERIFY_ARE_EQUAL(str, value.serialize());
    }

    TEST(negative_is_tests)
    {
        json::value b(true);
        json::value str(U("string"));
        json::value d(22.5);
        json::value n;
        json::value a = json::value::array(2);
        json::value o = json::value::object();

        VERIFY_IS_FALSE(b.is_number());
        VERIFY_IS_FALSE(str.is_boolean());
        VERIFY_IS_FALSE(d.is_string());
        VERIFY_IS_FALSE(a.is_object());
        VERIFY_IS_FALSE(o.is_array());
        VERIFY_IS_FALSE(n.is_string());
        VERIFY_IS_FALSE(str.is_null());
    }

    TEST(negative_index_operator_boolean)
    {
        json::value v = json::value::boolean(true);

        VERIFY_THROWS(v[0], json::json_exception);
        VERIFY_THROWS(v[U("H")], json::json_exception);
        VERIFY_THROWS(v[U("A")], json::json_exception);
    }

    TEST(negative_get_field_object)
    {
        json::value v;

        v[U("a")] = json::value::number(1);
        VERIFY_IS_TRUE(v.is_object());
        VERIFY_ARE_EQUAL(v[U("a")].as_integer(), 1);
        VERIFY_IS_TRUE(v[U("b")].is_null());
        VERIFY_THROWS(v[0], json::json_exception);
    }

    TEST(negative_get_element_array)
    {
        json::value v;

        v[0] = json::value::number(1);
        VERIFY_ARE_EQUAL(v[0].as_integer(), 1);
        VERIFY_IS_TRUE(v[1].is_null());
        VERIFY_THROWS(v[U("a")], json::json_exception);
    }

    TEST(has_field_object)
    {
        json::value v1;

        v1[U("a")] = json::value::number(1);
        v1[U("b")] = json::value::boolean(true);
        v1[U("c")] = json::value::string(U("a string"));
        v1[U("d")] = json::value::array({});
        json::value sub_field;
        sub_field[U("x")] = json::value::number(1);
        v1[U("e")] = sub_field;

        VERIFY_IS_TRUE(v1.has_field(U("a")));
        VERIFY_IS_TRUE(v1.has_field(U("b")));
        VERIFY_IS_TRUE(v1.has_field(U("c")));
        VERIFY_IS_TRUE(v1.has_field(U("d")));
        VERIFY_IS_TRUE(v1.has_field(U("e")));
        VERIFY_IS_FALSE(v1.has_field(U("f")));

        VERIFY_IS_TRUE(v1.has_number_field(U("a")));
        VERIFY_IS_TRUE(v1.has_integer_field(U("a")));
        VERIFY_IS_FALSE(v1.has_double_field(U("a")));
        VERIFY_IS_FALSE(v1.has_boolean_field(U("a")));
        VERIFY_IS_FALSE(v1.has_string_field(U("a")));
        VERIFY_IS_FALSE(v1.has_array_field(U("a")));
        VERIFY_IS_FALSE(v1.has_object_field(U("a")));

        VERIFY_IS_TRUE(v1.has_boolean_field(U("b")));
        VERIFY_IS_FALSE(v1.has_number_field(U("b")));
        VERIFY_IS_FALSE(v1.has_integer_field(U("b")));
        VERIFY_IS_FALSE(v1.has_double_field(U("b")));
        VERIFY_IS_FALSE(v1.has_string_field(U("b")));
        VERIFY_IS_FALSE(v1.has_array_field(U("b")));
        VERIFY_IS_FALSE(v1.has_object_field(U("b")));

        VERIFY_IS_TRUE(v1.has_string_field(U("c")));
        VERIFY_IS_FALSE(v1.has_boolean_field(U("c")));
        VERIFY_IS_FALSE(v1.has_number_field(U("c")));
        VERIFY_IS_FALSE(v1.has_integer_field(U("c")));
        VERIFY_IS_FALSE(v1.has_double_field(U("c")));
        VERIFY_IS_FALSE(v1.has_array_field(U("c")));
        VERIFY_IS_FALSE(v1.has_object_field(U("c")));

        VERIFY_IS_TRUE(v1.has_array_field(U("d")));
        VERIFY_IS_FALSE(v1.has_string_field(U("d")));
        VERIFY_IS_FALSE(v1.has_boolean_field(U("d")));
        VERIFY_IS_FALSE(v1.has_number_field(U("d")));
        VERIFY_IS_FALSE(v1.has_integer_field(U("d")));
        VERIFY_IS_FALSE(v1.has_double_field(U("d")));
        VERIFY_IS_FALSE(v1.has_object_field(U("d")));

        VERIFY_IS_TRUE(v1.has_object_field(U("e")));
        VERIFY_IS_FALSE(v1.has_array_field(U("e")));
        VERIFY_IS_FALSE(v1.has_string_field(U("e")));
        VERIFY_IS_FALSE(v1.has_boolean_field(U("e")));
        VERIFY_IS_FALSE(v1.has_number_field(U("e")));
        VERIFY_IS_FALSE(v1.has_integer_field(U("e")));
        VERIFY_IS_FALSE(v1.has_double_field(U("e")));

        json::value v2;

        v2[0] = json::value::number(1);
        VERIFY_IS_FALSE(v2.has_field(U("0")));
        VERIFY_IS_FALSE(v2.has_field(U("b")));
    }

    TEST(negative_as_tests)
    {
        json::value b(true);
        VERIFY_THROWS(b.as_double(), json::json_exception);
        VERIFY_THROWS(b.as_integer(), json::json_exception);
        VERIFY_THROWS(b.as_string(), json::json_exception);

        json::value str = json::value::string(U("string"));
        VERIFY_THROWS(str.as_double(), json::json_exception);
        VERIFY_THROWS(str.as_bool(), json::json_exception);
        VERIFY_THROWS(str.as_integer(), json::json_exception);

        json::value d(2.0f);
        VERIFY_THROWS(d.as_string(), json::json_exception);
        VERIFY_THROWS(d.as_bool(), json::json_exception);

        json::value i(11);
        VERIFY_THROWS(i.as_bool(), json::json_exception);
        VERIFY_THROWS(i.as_string(), json::json_exception);
    }

    TEST(erase_array_index)
    {
        json::value a = json::value::array(4);
        a[0] = json::value(1);
        a[1] = json::value(2);
        a[2] = json::value(3);
        a[3] = json::value(4);

        a.erase(1);
        VERIFY_ARE_EQUAL(3, a.size());
        VERIFY_ARE_EQUAL(1, a[0].as_integer());
        VERIFY_ARE_EQUAL(3, a[1].as_integer());
        VERIFY_ARE_EQUAL(4, a[2].as_integer());
        a.as_array().erase(2);
        VERIFY_ARE_EQUAL(2, a.size());
        VERIFY_ARE_EQUAL(1, a[0].as_integer());
        VERIFY_ARE_EQUAL(3, a[1].as_integer());
    }

    TEST(erase_array_iter)
    {
        json::value a = json::value::array(3);
        a[0] = json::value(1);
        a[1] = json::value(2);
        a[2] = json::value(3);

        auto iter = a.as_array().begin() + 1;
        auto afterLoc = a.as_array().erase(iter);
        VERIFY_ARE_EQUAL(3, afterLoc->as_integer());
        VERIFY_ARE_EQUAL(2, a.size());
        VERIFY_ARE_EQUAL(1, a[0].as_integer());
        VERIFY_ARE_EQUAL(3, a[1].as_integer());

        iter = a.as_array().begin() + 1;
        afterLoc = a.as_array().erase(iter);
        VERIFY_ARE_EQUAL(a.as_array().end(), afterLoc);
        VERIFY_ARE_EQUAL(1, a.size());
        VERIFY_ARE_EQUAL(1, a[0].as_integer());
    }

    TEST(erase_object_key)
    {
        auto o = json::value::object();
        o[U("a")] = json::value(1);
        o[U("b")] = json::value(2);
        o[U("c")] = json::value(3);
        o[U("d")] = json::value(4);

        o.erase(U("a"));
        VERIFY_ARE_EQUAL(3, o.size());
        VERIFY_ARE_EQUAL(2, o[U("b")].as_integer());
        VERIFY_ARE_EQUAL(3, o[U("c")].as_integer());
        VERIFY_ARE_EQUAL(4, o[U("d")].as_integer());

        o.as_object().erase(U("d"));
        VERIFY_ARE_EQUAL(2, o.size());
        VERIFY_ARE_EQUAL(2, o[U("b")].as_integer());
        VERIFY_ARE_EQUAL(3, o[U("c")].as_integer());
    }

    TEST(erase_object_iter)
    {
        auto o = json::value::object();
        o[U("a")] = json::value(1);
        o[U("b")] = json::value(2);
        o[U("c")] = json::value(3);
        o[U("d")] = json::value(4);

        auto iter = o.as_object().begin() + 1;
        auto afterLoc = o.as_object().erase(iter);
        VERIFY_ARE_EQUAL(3, o.size());
        VERIFY_ARE_EQUAL(3, afterLoc->second.as_integer());
        VERIFY_ARE_EQUAL(1, o[U("a")].as_integer());
        VERIFY_ARE_EQUAL(3, o[U("c")].as_integer());
        VERIFY_ARE_EQUAL(4, o[U("d")].as_integer());

        iter = o.as_object().begin() + 2;
        afterLoc = o.as_object().erase(iter);
        VERIFY_ARE_EQUAL(2, o.size());
        VERIFY_ARE_EQUAL(o.as_object().end(), afterLoc);
        VERIFY_ARE_EQUAL(1, o[U("a")].as_integer());
        VERIFY_ARE_EQUAL(3, o[U("c")].as_integer());
    }

    TEST(floating_number_serialize)
    {
        // This number will have the longest serializaton possible (lenght of the string):
        // Sign, exponent, decimal comma, longest mantisa and exponent make so.
        auto value = json::value(-3.123456789012345678901234567890E-123);

        // #digits + 2 to avoid loss + 1 for the sign + 1 for decimal point + 5 for exponent (e+xxx)
        const auto len = std::numeric_limits<double>::digits10 + 9;

        // Check narrow string implementation
        std::stringstream ss;
        value.serialize(ss);
        VERIFY_ARE_EQUAL(len, ss.str().length());

#ifdef _WIN32
        // Check wide string implementation
        std::basic_stringstream<wchar_t> wss;
        value.serialize(wss);
        VERIFY_ARE_EQUAL(len, wss.str().length());
#endif
    }

} // SUITE(to_as_and_operators_tests)

} // namespace json_tests
} // namespace functional
} // namespace tests
