/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Tests for JSON parsing.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "cpprest/json.h"
#include "unittestpp.h"
#include <array>
#include <iomanip>

#if defined(_WIN32) || defined(__APPLE__)
#include <regex>
#elif (defined(ANDROID) || defined(__ANDROID__))
#else
// GCC 4.8 doesn't support regex very well, fall back to Boost. Revist in GCC 4.9.
#include <boost/regex.hpp>
#endif

using namespace web;
using namespace utility;
using namespace utility::conversions;

static utility::string_t youtubeJson = _XPLATSTR(
R"delimeter({
 "kind": "youtube#playlistItemListResponse",
 "etag": "\"Fznwjl6JEQdo1MGvHOGaz_YanRU/ranGcWzseanYs9xZ0NXAq24qK-w\"",
 "pageInfo": {
  "totalResults": 3,
  "resultsPerPage": 5
 },
 "items": [
  {
   "kind": "youtube#playlistItem",
   "etag": "\"Fznwjl6JEQdo1MGvHOGaz_YanRU/phfRXORDKFrYjeJGWbI8MbIk08A\"",
   "id": "VVVGMWhNVVZ3bHJ2bFZNalVHT1pFeGdnLm12RERIeEJyd1U4",
   "snippet": {
    "publishedAt": "2013-05-24T22:03:10.000Z",
    "channelId": "UCF1hMUVwlrvlVMjUGOZExgg",
    "title": "C++ REST SDK (\"Casablanca\")",
    "description": "This library is a Microsoft effort to support cloud-based client-server communication in native code using a modern asynchronous C++ API design.",
    "thumbnails": {
     "default": {
      "url": "https://i.ytimg.com/vi/mvDDHxBrwU8/default.jpg",
      "width": 120,
      "height": 90
     },
     "medium": {
      "url": "https://i.ytimg.com/vi/mvDDHxBrwU8/mqdefault.jpg",
      "width": 320,
      "height": 180
     },
     "high": {
      "url": "https://i.ytimg.com/vi/mvDDHxBrwU8/hqdefault.jpg",
      "width": 480,
      "height": 360
     },
     "standard": {
      "url": "https://i.ytimg.com/vi/mvDDHxBrwU8/sddefault.jpg",
      "width": 640,
      "height": 480
     },
     "maxres": {
      "url": "https://i.ytimg.com/vi/mvDDHxBrwU8/maxresdefault.jpg",
      "width": 1280,
      "height": 720
     }
    },
    "channelTitle": "casablancacore",
    "playlistId": "UUF1hMUVwlrvlVMjUGOZExgg",
    "position": 0,
    "resourceId": {
     "kind": "youtube#video",
     "videoId": "mvDDHxBrwU8"
    }
   }
  },
  {
   "kind": "youtube#playlistItem",
   "etag": "\"Fznwjl6JEQdo1MGvHOGaz_YanRU/J65jYO0AIlbIqd4JpVigajlhVnE\"",
   "id": "VVVGMWhNVVZ3bHJ2bFZNalVHT1pFeGdnLmlFVU9fdDhFYW5r",
   "snippet": {
    "publishedAt": "2013-05-07T18:47:24.000Z",
    "channelId": "UCF1hMUVwlrvlVMjUGOZExgg",
    "title": "C++ REST SDK",
    "description": "A brief introduction to the C++ REST SDK. This video goes over high level concepts and features of the library. \n\nFor more information visit: http://casablanca.codeplex.com\nFor more information on PPL tasks visit: http://msdn.microsoft.com/en-us/library/dd492418.aspx",
    "thumbnails": {
     "default": {
      "url": "https://i.ytimg.com/vi/iEUO_t8Eank/default.jpg",
      "width": 120,
      "height": 90
     },
     "medium": {
      "url": "https://i.ytimg.com/vi/iEUO_t8Eank/mqdefault.jpg",
      "width": 320,
      "height": 180
     },
     "high": {
      "url": "https://i.ytimg.com/vi/iEUO_t8Eank/hqdefault.jpg",
      "width": 480,
      "height": 360
     },
     "standard": {
      "url": "https://i.ytimg.com/vi/iEUO_t8Eank/sddefault.jpg",
      "width": 640,
      "height": 480
     }
    },
    "channelTitle": "casablancacore",
    "playlistId": "UUF1hMUVwlrvlVMjUGOZExgg",
    "position": 1,
    "resourceId": {
     "kind": "youtube#video",
     "videoId": "iEUO_t8Eank"
    }
   }
  },
  {
   "kind": "youtube#playlistItem",
   "etag": "\"Fznwjl6JEQdo1MGvHOGaz_YanRU/XMpuK2N4-LOhDWtgCG8nBw7eNl8\"",
   "id": "VVVGMWhNVVZ3bHJ2bFZNalVHT1pFeGdnLk41cnlJN3U5RVFB",
   "snippet": {
    "publishedAt": "2013-05-02T21:24:56.000Z",
    "channelId": "UCF1hMUVwlrvlVMjUGOZExgg",
    "title": "bunny",
    "description": "",
    "thumbnails": {
     "default": {
      "url": "https://i.ytimg.com/vi/N5ryI7u9EQA/default.jpg",
      "width": 120,
      "height": 90
     },
     "medium": {
      "url": "https://i.ytimg.com/vi/N5ryI7u9EQA/mqdefault.jpg",
      "width": 320,
      "height": 180
     },
     "high": {
      "url": "https://i.ytimg.com/vi/N5ryI7u9EQA/hqdefault.jpg",
      "width": 480,
      "height": 360
     },
     "standard": {
      "url": "https://i.ytimg.com/vi/N5ryI7u9EQA/sddefault.jpg",
      "width": 640,
      "height": 480
     }
    },
    "channelTitle": "casablancacore",
    "playlistId": "UUF1hMUVwlrvlVMjUGOZExgg",
    "position": 2,
    "resourceId": {
     "kind": "youtube#video",
     "videoId": "N5ryI7u9EQA"
    }
   }
  }
 ]
})delimeter"
);

namespace tests
{
namespace functional
{
namespace json_tests
{
inline bool verify_parsing_error_msg(const std::string& str)
{
#if defined(_WIN32) || defined(__APPLE__)
    auto spattern = "^\\* Line \\d+, Column \\d+ Syntax error: .+";
    static std::regex pattern(spattern);
    return std::regex_match(str, pattern, std::regex_constants::match_flag_type::match_not_null);
#elif (defined(ANDROID) || defined(__ANDROID__))
    return str.find("Syntax error: ") != std::string::npos;
#else
    auto spattern = "^\\* Line \\d+, Column \\d+ Syntax error: .+";
    static boost::regex pattern(spattern);
    return boost::regex_match(str, pattern, boost::regex_constants::match_flag_type::match_not_null);
#endif
}

#if defined(_MSC_VER)
#pragma warning(disable : 4127) // const expression
#endif
#define VERIFY_PARSING_THROW(target)                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            target;                                                                                                    \
            VERIFY_IS_TRUE(false);                                                                                     \
        }                                                                                                              \
        catch (const json::json_exception& e)                                                                          \
        {                                                                                                              \
            VERIFY_IS_TRUE(verify_parsing_error_msg(e.what()));                                                        \
        }                                                                                                              \
        catch (...)                                                                                                    \
        {                                                                                                              \
            VERIFY_IS_TRUE(false);                                                                                     \
        }                                                                                                              \
    } while (false)

SUITE(parsing_tests)
{
    TEST(stringstream_t)
    {
        utility::stringstream_t ss0;
        ss0 << U("null");
        json::value v0 = json::value::parse(ss0);

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
        ss8 << U("{ \"a\" : 10 }");
        json::value v8 = json::value::parse(ss8);

        utility::stringstream_t ss9;
        ss9 << U("[1,2,3,true]");
        json::value v9 = json::value::parse(ss9);

        VERIFY_ARE_EQUAL(v1.type(), json::value::Number);
        VERIFY_ARE_EQUAL(v2.type(), json::value::Number);
        VERIFY_ARE_EQUAL(v3.type(), json::value::Boolean);
        VERIFY_ARE_EQUAL(v4.type(), json::value::String);
        VERIFY_ARE_EQUAL(v8.type(), json::value::Object);
        VERIFY_ARE_EQUAL(v9.type(), json::value::Array);
    }

    TEST(whitespace_failure) { VERIFY_PARSING_THROW(json::value::parse(U("  "))); }

    static const std::array<char, 4> whitespace_chars = {{0x20, 0x09, 0x0A, 0x0D}};

    TEST(whitespace_array)
    {
        // Try all the whitespace characters before/after all the structural characters
        // whitespace characters according to RFC4627: space, horizontal tab, line feed or new line, carriage return
        // structural characters: [{]}:,

        // [,]
        for (auto ch : whitespace_chars)
        {
            utility::string_t input;
            input.append(2, ch);
            input.append(U("["));
            input.append(2, ch);
            input.append(U("1"));
            input.append(1, ch);
            input.append(U(","));
            input.append(4, ch);
            input.append(U("2"));
            input.append(1, ch);
            input.append(U("]"));
            input.append(2, ch);
            json::value val = json::value::parse(input);
            VERIFY_IS_TRUE(val.is_array());
            VERIFY_ARE_EQUAL(U("1"), val[0].serialize());
            VERIFY_ARE_EQUAL(U("2"), val[1].serialize());
        }
    }

    TEST(whitespace_object)
    {
        // {:}
        for (auto ch : whitespace_chars)
        {
            utility::string_t input;
            input.append(2, ch);
            input.append(U("{"));
            input.append(2, ch);
            input.append(U("\"1\""));
            input.append(1, ch);
            input.append(U(":"));
            input.append(4, ch);
            input.append(U("2"));
            input.append(1, ch);
            input.append(U("}"));
            input.append(2, ch);
            json::value val = json::value::parse(input);
            VERIFY_IS_TRUE(val.is_object());
            VERIFY_ARE_EQUAL(U("2"), val[U("1")].serialize());
        }
    }

    TEST(string_t)
    {
        json::value str = json::value::parse(U("\"\\\"\""));
        VERIFY_ARE_EQUAL(U("\""), str.as_string());

        str = json::value::parse(U("\"\""));
        VERIFY_ARE_EQUAL(U(""), str.as_string());

        str = json::value::parse(U("\"\\\"ds\""));
        VERIFY_ARE_EQUAL(U("\"ds"), str.as_string());

        str = json::value::parse(U("\"\\\"\\\"\""));
        VERIFY_ARE_EQUAL(U("\"\""), str.as_string());

        // two character escapes
        str = json::value::parse(U("\"\\\\\""));
        VERIFY_ARE_EQUAL(U("\\"), str.as_string());

        str = json::value::parse(U("\"\\/\""));
        VERIFY_ARE_EQUAL(U("/"), str.as_string());

        str = json::value::parse(U("\"\\b\""));
        VERIFY_ARE_EQUAL(U("\b"), str.as_string());

        str = json::value::parse(U("\"\\f\""));
        VERIFY_ARE_EQUAL(U("\f"), str.as_string());

        str = json::value::parse(U("\"\\n\""));
        VERIFY_ARE_EQUAL(U("\n"), str.as_string());

        str = json::value::parse(U("\"\\r\""));
        VERIFY_ARE_EQUAL(U("\r"), str.as_string());

        str = json::value::parse(U("\"\\t\""));
        VERIFY_ARE_EQUAL(U("\t"), str.as_string());
    }

    TEST(escaped_unicode_string)
    {
        auto str = json::value::parse(U("\"\\u0041\""));
        VERIFY_ARE_EQUAL(U("A"), str.as_string());

        str = json::value::parse(U("\"\\u004B\""));
        VERIFY_ARE_EQUAL(U("K"), str.as_string());

        str = json::value::parse(U("\"\\u20AC\""));
        // Euro sign as a hexadecimal UTF-8
        const auto euro = to_string_t("\xE2\x82\xAC");
        VERIFY_ARE_EQUAL(euro, str.as_string());

        // UTF-16 character with surrogate pair
        str = json::value::parse(U("\"\\ud83d\\ude00\""));
        // Grinning Face emoji as a hexadecimal UTF-8
        const auto emoji = to_string_t("\xF0\x9F\x98\x80");
        VERIFY_ARE_EQUAL(emoji, str.as_string());

        VERIFY_PARSING_THROW(json::value::parse(U("\"\\u0klB\"")));
    }

    TEST(escaping_control_characters)
    {
        std::vector<int> chars;
        for (int i = 0; i <= 0x1F; ++i)
        {
            chars.push_back(i);
        }
        chars.push_back(0x5C); // backslash '\'
        chars.push_back(0x22); // quotation '"'

        for (int i : chars)
        {
            utility::stringstream_t ss;
            ss << U("\"\\u") << std::uppercase << std::setfill(U('0')) << std::setw(4) << std::hex << i << U("\"");
            const auto& str = ss.str();
            auto expectedStr = str;
            if (i == 0x08)
            {
                expectedStr = U("\"\\b\"");
            }
            else if (i == 0x09)
            {
                expectedStr = U("\"\\t\"");
            }
            else if (i == 0x0A)
            {
                expectedStr = U("\"\\n\"");
            }
            else if (i == 0x0C)
            {
                expectedStr = U("\"\\f\"");
            }
            else if (i == 0x0D)
            {
                expectedStr = U("\"\\r\"");
            }
            else if (i == 0x5C)
            {
                expectedStr = U("\"\\\\\"");
            }
            else if (i == 0x22)
            {
                expectedStr = U("\"\\\"\"");
            }

            // Try constructing a json string value directly.
            utility::string_t schar;
            schar.push_back(static_cast<utility::string_t::value_type>(i));
            const auto& sv = json::value::string(schar);
            VERIFY_ARE_EQUAL(expectedStr, sv.serialize());

            // Try parsing a string
            const auto& v = json::value::parse(str);
            VERIFY_IS_TRUE(v.is_string());
            VERIFY_ARE_EQUAL(expectedStr, v.serialize());

            // Try parsing a stringstream.
            const auto& ssv = json::value::parse(ss);
            VERIFY_ARE_EQUAL(expectedStr, ssv.serialize());
        }
    }

    TEST(comments_string)
    {
        // Nothing but a comment
        VERIFY_PARSING_THROW(json::value::parse(U(" /* There's nothing but a comment here */  ")));
        VERIFY_PARSING_THROW(json::value::parse(U(" // There's nothing but a comment here\n")));

        // Some invalid comments
        VERIFY_PARSING_THROW(json::value::parse(U(" -22 /*/")));
        VERIFY_PARSING_THROW(json::value::parse(U(" -22 /* /* nested */ */")));

        // Correctly placed comments
        json::value num1 = json::value::parse(U("-22 // This is a trailing comment\n"));
        VERIFY_ARE_EQUAL(-22, num1.as_double());
        num1 = json::value::parse(U(" -22 /* This is a trailing comment with a // nested\n comment */"));
        VERIFY_ARE_EQUAL(-22, num1.as_double());
        json::value num2 = json::value::parse(U("// This is a leading comment\n -22"));
        VERIFY_ARE_EQUAL(-22, num2.as_double());
        json::value num3 = json::value::parse(U("-22 /* This is a trailing comment */"));
        VERIFY_ARE_EQUAL(-22, num3.as_double());
        json::value num4 = json::value::parse(U("/* This is a leading comment */ -22"));
        VERIFY_ARE_EQUAL(-22, num4.as_double());
        json::value num5 = json::value::parse(U("-22 /***/"));
        VERIFY_ARE_EQUAL(-22, num5.as_double());

        json::value obj1 = json::value::parse(U("{// A comment in the middle of an empty object\n}"));
        VERIFY_IS_TRUE(obj1.is_object());
        VERIFY_ARE_EQUAL(0u, obj1.size());
        json::value obj2 = json::value::parse(U("{/* A comment in the middle of an empty object */}"));
        VERIFY_IS_TRUE(obj2.is_object());
        VERIFY_ARE_EQUAL(0u, obj2.size());
        json::value obj3 = json::value::parse(U("{ \"test\" : // A comment in the middle of a non-empty object\n 2}"));
        VERIFY_IS_TRUE(obj3.is_object());
        VERIFY_ARE_EQUAL(1u, obj3.size());
        json::value obj4 = json::value::parse(U("{ \"test\" : /* A comment in the middle of a non-empty object */ 2}"));
        VERIFY_IS_TRUE(obj4.is_object());
        VERIFY_ARE_EQUAL(1u, obj4.size());

        json::value arr1 = json::value::parse(U("[// A comment in the middle of an empty array\n]"));
        VERIFY_IS_TRUE(arr1.is_array());
        VERIFY_ARE_EQUAL(0u, arr1.size());
        json::value arr2 = json::value::parse(U("[/* A comment in the middle of an empty array */]"));
        VERIFY_IS_TRUE(arr2.is_array());
        VERIFY_ARE_EQUAL(0u, arr2.size());
        json::value arr3 = json::value::parse(U("[ 1, // A comment in the middle of a non-array\n 2]"));
        VERIFY_IS_TRUE(arr3.is_array());
        VERIFY_ARE_EQUAL(2u, arr3.size());
        json::value arr4 = json::value::parse(U("[ 1, /* A comment in the middle of a non-empty array */ 2]"));
        VERIFY_IS_TRUE(arr4.is_array());
        VERIFY_ARE_EQUAL(2u, arr4.size());
    }

    TEST(comments_stream)
    {
        // Nothing but a comment
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U(" /* There's nothing but a comment here */ ");
            VERIFY_PARSING_THROW(json::value::parse(stream));
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U(" // There's nothing but a comment here\n ");
            VERIFY_PARSING_THROW(json::value::parse(stream));
        }

        // Some invalid comments
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U(" -22 /*/");
            VERIFY_PARSING_THROW(json::value::parse(stream));
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U(" -22 /* /* nested */ */");
            VERIFY_PARSING_THROW(json::value::parse(stream));
        }

        // Correctly placed comments
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("-22 // This is a trailing comment\n");
            json::value num1 = json::value::parse(stream);
            VERIFY_ARE_EQUAL(-22, num1.as_double());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U(" -22 /* This is a trailing comment with a // nested\n comment */");
            json::value num1 = json::value::parse(stream);
            VERIFY_ARE_EQUAL(-22, num1.as_double());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("// This is a leading comment\n -22");
            json::value num2 = json::value::parse(stream);
            VERIFY_ARE_EQUAL(-22, num2.as_double());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("-22 /* This is a trailing comment */");
            json::value num3 = json::value::parse(stream);
            VERIFY_ARE_EQUAL(-22, num3.as_double());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("/* This is a leading comment */ -22");
            json::value num4 = json::value::parse(stream);
            VERIFY_ARE_EQUAL(-22, num4.as_double());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("-22 /***/");
            json::value num4 = json::value::parse(stream);
            VERIFY_ARE_EQUAL(-22, num4.as_double());
        }

        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("{// A comment in the middle of an empty object\n}");
            json::value obj1 = json::value::parse(stream);
            VERIFY_IS_TRUE(obj1.is_object());
            VERIFY_ARE_EQUAL(0u, obj1.size());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("{/* A comment in the middle of an empty object */}");
            json::value obj2 = json::value::parse(stream);
            VERIFY_IS_TRUE(obj2.is_object());
            VERIFY_ARE_EQUAL(0u, obj2.size());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("{ \"test1\" : // A comment in the middle of a non-empty object\n 2}");
            json::value obj3 = json::value::parse(stream);
            VERIFY_IS_TRUE(obj3.is_object());
            VERIFY_ARE_EQUAL(1u, obj3.size());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("{ \"test1\" : /* A comment in the middle of a non-empty object */ 2}");
            json::value obj4 = json::value::parse(stream);
            VERIFY_IS_TRUE(obj4.is_object());
            VERIFY_ARE_EQUAL(1u, obj4.size());
        }

        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("[// A comment in the middle of an empty array\n]");
            json::value arr1 = json::value::parse(stream);
            VERIFY_IS_TRUE(arr1.is_array());
            VERIFY_ARE_EQUAL(0u, arr1.size());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("[/* A comment in the middle of an empty array */]");
            json::value arr2 = json::value::parse(stream);
            VERIFY_IS_TRUE(arr2.is_array());
            VERIFY_ARE_EQUAL(0u, arr2.size());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("[ 1, // A comment in the middle of a non-array\n 2]");
            json::value arr3 = json::value::parse(stream);
            VERIFY_IS_TRUE(arr3.is_array());
            VERIFY_ARE_EQUAL(2u, arr3.size());
        }
        {
            std::basic_stringstream<utility::char_t> stream;
            stream << U("[ 1, /* A comment in the middle of a non-empty array */ 2]");
            json::value arr4 = json::value::parse(stream);
            VERIFY_IS_TRUE(arr4.is_array());
            VERIFY_ARE_EQUAL(2u, arr4.size());
        }
    }

    TEST(empty_object_array)
    {
        json::value obj = json::value::parse(U("{}"));
        VERIFY_IS_TRUE(obj.is_object());
        VERIFY_ARE_EQUAL(0u, obj.size());

        json::value arr = json::value::parse(U("[]"));
        VERIFY_IS_TRUE(arr.is_array());
        VERIFY_ARE_EQUAL(0u, arr.size());
    }

    TEST(bug_object_field_key_no_value)
    {
        VERIFY_PARSING_THROW(json::value::parse(U("{\"meow\"}")));
        VERIFY_PARSING_THROW(json::value::parse(U("{\"meow\": 42, \"purr\": 57, \"hiss\"}")));
    }

    TEST(bug_416116)
    {
        json::value data2 = json::value::parse(U("\"δοκιμή\""));
        auto s = data2.serialize();

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4566)
#endif
        VERIFY_ARE_EQUAL(s, U("\"δοκιμή\""));
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    }

    TEST(byte_ptr_parsing_array)
    {
        char s[] = "[ \"test1\",true]";
        json::value v = json::value::parse(s);
        std::stringstream ss;
        ss << s;
        json::value vv = json::value::parse(ss);
        VERIFY_ARE_EQUAL(v, vv);

        auto s2 = v.serialize();
        VERIFY_ARE_EQUAL(s2, U("[\"test1\",true]"));

        std::stringstream os;
        vv.serialize(os);
        VERIFY_ARE_EQUAL(s2, to_string_t(os.str()));
    }

    TEST(byte_ptr_parsing_object)
    {
        char s[] = "{\"test1\":true }";
        json::value v = json::value::parse(s);
        std::stringstream ss;
        ss << s;
        json::value vv = json::value::parse(ss);
        VERIFY_ARE_EQUAL(v, vv);

        auto s2 = v.serialize();
        VERIFY_ARE_EQUAL(s2, U("{\"test1\":true}"));

        std::stringstream os;
        vv.serialize(os);
        VERIFY_ARE_EQUAL(s2, to_string_t(os.str()));
    }

    TEST(Japanese)
    {
        utility::string_t ws = U("\"こんにちは\"");
        std::string s = to_utf8string(ws);
        json::value v = json::value::parse(s);

        std::stringstream ss;
        ss << s;
        json::value vv = json::value::parse(ss);
        VERIFY_ARE_EQUAL(v, vv);

        auto s2 = v.serialize();
        VERIFY_ARE_EQUAL(s2, ws);

        std::stringstream os;
        vv.serialize(os);
        VERIFY_ARE_EQUAL(s2, to_string_t(os.str()));
    }

    TEST(Russian)
    {
        utility::string_t ws = U("{\"results\":[{\"id\":272655310,\"name\":\"Андрей Ив´анов\"}]}");
        json::value v1 = json::value::parse(ws);
        auto s2 = v1.serialize();

        VERIFY_ARE_EQUAL(s2, ws);

        std::string s = to_utf8string(ws);

        std::stringstream ss;
        ss << s;
        json::value v2 = json::value::parse(ss);
        auto s3 = v2.serialize();

        VERIFY_ARE_EQUAL(s3, ws);
    }

    utility::string_t make_deep_json_string(size_t depth)
    {
        utility::string_t strval;
        for (size_t i = 0; i < depth; ++i)
        {
            strval += U("{ \"a\" : 10, \"b\" : ");
        }
        strval += U("20");
        for (size_t i = 0; i < depth; ++i)
        {
            strval += U("}");
        }
        return strval;
    }

    TEST(deeply_nested)
    {
#if defined(__APPLE__)
        size_t safeDepth = 32;
        size_t overDepth = 33;
#else
        size_t safeDepth = 128;
        size_t overDepth = 129;
#endif

        // This should parse without issues:
        auto strGood = make_deep_json_string(safeDepth);
        json::value::parse(strGood);

        // But this one should throw:
        auto strBad = make_deep_json_string(overDepth);
        VERIFY_PARSING_THROW(json::value::parse(strBad));
    }

    static bool compare_pairs(const std::pair<utility::string_t, json::value>& p1,
                              const std::pair<utility::string_t, json::value>& p2)
    {
        return p1.first < p2.first;
    }

    TEST(unsorted_object_parsing)
    {
        utility::stringstream_t ss;
        ss << U("{\"z\":2, \"a\":1}");
        json::value v = json::value::parse(ss);
        auto& obj = v.as_object();

        VERIFY_ARE_NOT_EQUAL(obj.find(U("a")), obj.end());
        VERIFY_ARE_NOT_EQUAL(obj.find(U("z")), obj.end());
        VERIFY_ARE_EQUAL(obj[U("a")], 1);
        VERIFY_ARE_EQUAL(obj[U("z")], 2);
        VERIFY_ARE_EQUAL(obj.size(), 2);

        VERIFY_IS_TRUE(::std::is_sorted(obj.begin(), obj.end(), compare_pairs));
    }

    TEST(keep_order_while_parsing)
    {
        utility::stringstream_t ss;
        ss << U("{\"k\":3, \"j\":2, \"i\":1}");

        json::keep_object_element_order(true);
        struct restore
        {
            ~restore() { json::keep_object_element_order(false); }
        } _;

        json::value v = json::value::parse(ss);
        auto& obj = v.as_object();

        // Make sure collection stays unsorted:
        auto b = obj.begin();
        VERIFY_ARE_EQUAL(b[0].first, U("k"));
        VERIFY_ARE_EQUAL(b[1].first, U("j"));
        VERIFY_ARE_EQUAL(b[2].first, U("i"));

        // Make sure lookup still works:
        auto val_i = obj[U("i")];
        VERIFY_ARE_EQUAL(val_i.as_integer(), 1);

        auto val_j = obj[U("j")];
        VERIFY_ARE_EQUAL(val_j.as_integer(), 2);

        // Make sure 'a' goes to the back of the collection, and
        // can be looked up
        obj[U("a")] = 4;
        b = obj.begin();
        VERIFY_ARE_EQUAL(b[3].first, U("a"));
        VERIFY_ARE_EQUAL(obj[U("a")].as_integer(), 4);
    }

    TEST(non_default_locale, "Ignore:Android", "Locale unsupported on Android")
    {
        std::string originalLocale = setlocale(LC_ALL, nullptr);
#ifdef _WIN32
        std::string changedLocale("fr-FR");
#else
        std::string changedLocale("fr_FR.utf8");
#endif

        // If locale isn't installed on system just silently pass.
        if (setlocale(LC_ALL, changedLocale.c_str()) != nullptr)
        {
            // string serialize
            utility::string_t str(U("[true,false,-1.55,5,null,{\"abc\":5555}]"));
            json::value v = json::value::parse(str);
            VERIFY_ARE_EQUAL(changedLocale, setlocale(LC_ALL, nullptr));
            VERIFY_ARE_EQUAL(str, v.serialize());
            VERIFY_ARE_EQUAL(changedLocale, setlocale(LC_ALL, nullptr));

            setlocale(LC_ALL, originalLocale.c_str());
            setlocale(LC_NUMERIC, changedLocale.c_str());

            // cpprestsdk stream serialize
            utility::stringstream_t stream;
            stream << v;
            utility::string_t serializedStr;
            stream >> serializedStr;
            VERIFY_ARE_EQUAL(str, serializedStr);

            // std stream serialize
            std::stringstream stdStream;
            v.serialize(stdStream);
            std::string stdStr;
            stdStream >> stdStr;
            VERIFY_ARE_EQUAL(str, utility::conversions::to_string_t(stdStr));

            setlocale(LC_ALL, originalLocale.c_str());
        }
    }

    template<typename T>
    void error_code_helper(T & jsonData)
    {
        std::error_code err;
        auto parsedObject = web::json::value::parse(jsonData, err);
        VERIFY_IS_TRUE(err.value() == 0);
        VERIFY_IS_TRUE(!parsedObject.is_null());
    }

    TEST(parse_overload_success)
    {
        std::error_code err;
        utility::string_t valueStr(U("\"JSONString\""));
        utility::string_t arrStr(U("[true,false,-1.55,5,null,{\"abc\":5555}]"));
        utility::string_t objStr(U("{\"k\":3, \"j\":2, \"i\":1}"));

        error_code_helper(valueStr);
        error_code_helper(arrStr);
        error_code_helper(objStr);

        utility::stringstream_t valueStringStream;
        utility::stringstream_t arrayStringStream;
        utility::stringstream_t objStringStream;

        valueStringStream << valueStr;
        arrayStringStream << arrStr;
        objStringStream << objStr;

        error_code_helper(valueStringStream);
        error_code_helper(arrayStringStream);
        error_code_helper(objStringStream);

#ifdef _WIN32
        std::wstringbuf buf;

        buf.sputn(valueStr.c_str(), valueStr.size());
        std::wistream valStream(&buf);
        error_code_helper(valStream);

        buf.sputn(arrStr.c_str(), arrStr.size());
        std::wistream arrStream(&buf);
        error_code_helper(arrStream);

        buf.sputn(objStr.c_str(), objStr.size());
        std::wistream objStream(&buf);
        error_code_helper(objStream);
#endif
    }

    TEST(parse_overload_failed)
    {
        std::error_code err, streamErr, iStreamErr;
        utility::string_t str(U("JSONString"));
        utility::string_t arrStr(U("[true, false"));
        json::value parsedObject = json::value::parse(str, err);

        VERIFY_IS_TRUE(err.value() > 0);
        VERIFY_IS_TRUE(parsedObject.is_null());

        utility::stringstream_t stream;
        stream << str;

        parsedObject = json::value::parse(arrStr, streamErr);
        VERIFY_IS_TRUE(streamErr.value() > 0);
        VERIFY_IS_TRUE(parsedObject.is_null());

#ifdef _WIN32
        std::wstringbuf buf;
        buf.sputn(str.c_str(), str.size());
        std::wistream iStream(&buf);
        parsedObject = json::value::parse(str, iStreamErr);
        VERIFY_IS_TRUE(iStreamErr.value() > 0);
        VERIFY_IS_TRUE(parsedObject.is_null());
#endif
    }

    TEST(youtube_api)
    {
        auto v = json::value::parse(youtubeJson);
        int count = 0;
        auto& obj = v.as_object();

        VERIFY_ARE_NOT_EQUAL(obj.find(U("pageInfo")), obj.end());
        VERIFY_ARE_NOT_EQUAL(obj.find(U("items")), obj.end());

        auto& items = obj[U("items")];

        for (auto iter = items.as_array().cbegin(); iter != items.as_array().cend(); ++iter)
        {
            const auto& item = *iter;
            auto iSnippet = item.as_object().find(U("snippet"));
            if (iSnippet == item.as_object().end())
            {
                throw std::runtime_error("snippet key not found");
            }
            auto iTitle = iSnippet->second.as_object().find(U("title"));
            if (iTitle == iSnippet->second.as_object().end())
            {
                throw std::runtime_error("title key not found");
            }
            auto name = iTitle->second.serialize();
            count++;
        }
        VERIFY_ARE_EQUAL(3, count); // Update this accordingly, if the number of items changes
    }

} // SUITE(parsing_tests)

} // namespace json_tests
} // namespace functional
} // namespace tests
