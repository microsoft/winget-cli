#include "pch.h"
#include "string_helpers.h"

using namespace std;
using namespace std::string_view_literals;

template <typename char_type>
struct valid_strings;

template <>
struct valid_strings<xlang_char8>
{
    static constexpr basic_string_view<xlang_char8> value[] = {
        {nullptr, 0},
        u8""sv,
        u8"A simple string."sv,
        u8"\u007f", // Largest one byte UTF8 sequence
        u8"\u0080", // Smallest two byte UTF8 sequence
        u8"\u07ff", // Largest two byte UTF8 sequence
        u8"\u0800", // Smallest three byte UTF8 sequence
        u8"\ud7ff", // highest code point below surrogate range
        u8"\ue000", // smallest code point above surrogate range
        u8"\uffff", // largest three byte UTF8 sequence
        u8"\U00010000", // smallest four byte UTF8 sequence
        u8"\U0010ffff", // largest Unicode code point
    };
};

template <>
struct valid_strings<char16_t>
{
    static constexpr basic_string_view<char16_t> value[] = {
        {nullptr, 0},
        u""sv,
        u"A simple string."sv,
        u"\u007f", // Largest one byte UTF8 sequence
        u"\u0080", // Smallest two byte UTF8 sequence
        u"\u07ff", // Largest two byte UTF8 sequence
        u"\u0800", // Smallest three byte UTF8 sequence
        u"\ud7ff", // highest code point below surrogate range
        u"\ue000", // smallest code point above surrogate range
        u"\uffff", // largest three byte UTF8 sequence
        u"\U00010000", // smallest four byte UTF8 sequence
        u"\U0010ffff", // largest Unicode code point
    };
};

template <typename char_type>
struct invalid_strings {};

template <>
struct invalid_strings<xlang_char8>
{
    // Due to VC++ bug that is misencoding /x escapes in u8-prefixed string literals, give these strings a u prefix for now.
    static constexpr basic_string_view<xlang_char8> value[] = {
        "\xf4\x90\x80\x80"sv, // U+00110000 minimum out of range
        "\xf7\xbf\xbf\xbf"sv, // U+001FFFFF maximum 4 byte encoding
        "\xf8\x88\x80\x80\x80"sv, // U+00200000 minimum 5 byte encoding
        "\xfb\xbf\xbf\xbf\xbf"sv, // U+03FFFFFF maximum 5 byte encoding
        "\xfc\x84\x80\x80\x80\x80"sv, // U+04000000 minimum 6 byte encoding
        "\xfd\xbf\xbf\xbf\xbf\xbf"sv, // U+07FFFFFF maximum 6 byte encoding

        //// Lone surrogates
        "\xed\xa0\x80"sv, // U+D800 
        "\xed\xad\xbd"sv, // U+DB7F
        "\xed\xae\x80"sv, // U+DB80
        "\xed\xaf\xbf"sv, // U+DBFF
        "\xed\xbe\x80"sv, // U+DF80
        "\xed\xbf\xbf"sv, // U+DFFF

        // Overlong encodings of the character '/'
        "\xc0\xaf"sv,
        "\xe0\x80\xaf"sv,
        "\xf0\x80\x80\xaf"sv,
        "\xf8\x80\x80\x80\xaf"sv,
        "\xfc\x80\x80\x80\x80\xaf"sv,

        // Overlong encodings of the nul character
        "\xc0\x80"sv,
        "\xe0\x80\x80"sv,
        "\xf0\x80\x80\x80"sv,
        "\xf8\x80\x80\x80\x80"sv,
        "\xfc\x80\x80\x80\x80\x80"sv,

        // Overlong encodings of other boundary code points
        "\xc0\x80"sv, // U+0001
        "\xc1\xbf"sv, // U+007F
        "\xe0\x82\x80"sv, // U+0080
        "\xe0\x9f\xbf"sv, // U+07FF
        "\xf0\x80\xc0\x80"sv, // U+0800
        "\xf0\x8f\xbf\xbf"sv, // U+FFFF
        "\xf8\x80\x90\x80\x80"sv, // U+00010000
        "\xf8\x84\x8f\xbf\xbf"sv, // U+0010FFFF
        "\xf8\x84\x90\x80\x80"sv, // U+00110000
        "\xf8\x87\xbf\xbf\xbf"sv, // U+001FFFFF
        "\xfc\x80\x88\x80\x80\x80"sv, // U+00200000
        "\xfc\x83\xbf\xbf\xbf\xbf"sv, // U+03FFFFFF
    };
};

template <>
struct invalid_strings<char16_t>
{
    static constexpr basic_string_view<char16_t> value[] = {
        // Lone surrogates
        u"\xd800"sv,
        u"\xdb7f"sv,
        u"\xdb80"sv,
        u"\xdbff"sv,
        u"\xdc00"sv,
        u"\xdf80"sv,
        u"\xdfff"sv,

        // Reversed surrogates
        u"\xdc00\xd800"sv,
        u"\xdc00\xdbff"sv,
        u"\xdfff\xd800"sv,
        u"\xdfff\xdbff"sv,
        u"?\xdc00\xd800"sv,
        u"?\xdc00\xdbff"sv,
        u"?\xdfff\xd800"sv,
        u"?\xdfff\xdbff"sv,
        u"\xdc00\xd800?"sv,
        u"\xdc00\xdbff?"sv,
        u"\xdfff\xd800?"sv,
        u"\xdfff\xdbff?"sv,
        u"?\xdc00\xd800?"sv,
        u"?\xdc00\xdbff?"sv,
        u"?\xdfff\xd800?"sv,
        u"?\xdfff\xdbff?"sv,

        // Broken surrogates
        u"\xd800?"sv,
        u"\xdbff?"sv,
        u"\xdc00?"sv,
        u"\xdfff?"sv,
        u"?\xd800"sv,
        u"?\xdbff"sv,
        u"?\xdc00"sv,
        u"?\xdfff"sv,

        // Interrupted surrogates
        u"?\xd800?\xd800?"sv,
        u"?\xdbff?\xd800?"sv,
        u"?\xdc00?\xd800?"sv,
        u"?\xdfff?\xd800?"sv,
        u"?\xd800?\xdbff?"sv,
        u"?\xdbff?\xdbff?"sv,
        u"?\xdc00?\xdbff?"sv,
        u"?\xdfff?\xdbff?"sv,
        u"?\xd800?\xdc00?"sv,
        u"?\xdbff?\xdc00?"sv,
        u"?\xdc00?\xdc00?"sv,
        u"?\xdfff?\xdc00?"sv,
        u"?\xd800?\xdfff?"sv,
        u"?\xdbff?\xdfff?"sv,
        u"?\xdc00?\xdfff?"sv,
        u"?\xdfff?\xdfff?"sv,
    };
};

template <typename T>
struct encoding {};

template <>
struct encoding<xlang_char8> : std::integral_constant<xlang_string_encoding, xlang_string_encoding::utf8> {};

template <>
struct encoding<char16_t> : std::integral_constant<xlang_string_encoding, xlang_string_encoding::utf16> {};

template <typename char_type>
inline bool has_encoding(xlang_string str) noexcept
{
    static_assert(std::is_same_v<char_type, xlang_char8> || std::is_same_v<char_type, char16_t>);
    auto const enc = xlang_get_string_encoding(str);
    return (enc & encoding<char_type>::value) != xlang_string_encoding::none;
}

template <typename T>
struct alternate_type
{};

template <>
struct alternate_type<xlang_char8>
{
    using type = char16_t;
};

template <>
struct alternate_type<char16_t>
{
    using type = xlang_char8;
};

template <typename char_type>
void simple_string()
{
    for (basic_string_view<char_type> const test_string : valid_strings<char_type>::value)
    {
        xlang_string str{};
        xlang_error_info* result{};

        {
            INFO("Create a valid string");
            result = xlang_create_string<char_type>(test_string.data(), static_cast<uint32_t>(test_string.size()), &str);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str));
        }

        char_type const* buffer{};
        uint32_t length{};

        {
            INFO("The buffer matches the supplied string");
            result = xlang_get_string_raw_buffer<char_type>(str, &buffer, &length);
            REQUIRE(result == nullptr);
            REQUIRE(test_string == basic_string_view<char_type>{buffer, length});
        }

        xlang_string str2{};
        {
            INFO("We can duplicate the string");
            result = xlang_duplicate_string(str, &str2);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str2));
        }

        char_type const* buffer2{};
        uint32_t length2{};

        {
            INFO("The duplicated string returns the same buffer");
            result = xlang_get_string_raw_buffer(str2, &buffer2, &length2);
            REQUIRE(result == nullptr);
            REQUIRE(buffer == buffer2);
            REQUIRE(length == length2);
        }

        {
            INFO("Even after deleting the original string");
            xlang_delete_string(str);

            result = xlang_get_string_raw_buffer(str2, &buffer2, &length2);
            REQUIRE(result == nullptr);
            REQUIRE(buffer == buffer2);
            REQUIRE(length == length2);

            xlang_delete_string(str2);
        }
    }
}

TEST_CASE("Simple UTF-8 strings")
{
    simple_string<xlang_char8>();
}

TEST_CASE("Simple UTF-16 strings")
{
    simple_string<char16_t>();
}

template <typename char_type>
void simple_string_reference()
{
    for (basic_string_view<char_type> const test_string : valid_strings<char_type>::value)
    {
        xlang_string str_ref{};
        xlang_error_info* result{};
        xlang_string_header header{};

        {
            INFO("Create a simple string reference");
            result = xlang_create_string_reference<char_type>(test_string.data(), static_cast<uint32_t>(test_string.size()), &header, &str_ref);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str_ref));
        }

        char_type const* buffer_ref{};
        uint32_t length_ref{};
        {
            INFO("Ensure the buffer contents match the supplied string");
            result = xlang_get_string_raw_buffer<char_type>(str_ref, &buffer_ref, &length_ref);
            REQUIRE(result == nullptr);
            REQUIRE(basic_string_view<char_type>{buffer_ref, length_ref} == test_string);
        }

        xlang_string str{};
        char_type const* buffer{};
        uint32_t length{};
        {
            INFO("Duplicate the string reference, and ensure the duplicated contents match");
            result = xlang_duplicate_string(str_ref, &str);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str));
            result = xlang_get_string_raw_buffer<char_type>(str, &buffer, &length);
            REQUIRE(result == nullptr);
            REQUIRE(test_string == basic_string_view<char_type>{buffer, length});
        }

        xlang_delete_string(str_ref);
        xlang_delete_string(str);
    }
}

TEST_CASE("Simple UTF-8 string references")
{
    simple_string_reference<xlang_char8>();
}

TEST_CASE("Simple UTF-16 string references")
{
    simple_string_reference<char16_t>();
}

template <typename char_type>
void simple_preallocated()
{
    for (basic_string_view<char_type> const test_string : valid_strings<char_type>::value)
    {
        xlang_error_info* result{};
        xlang_string_buffer buffer_handle{};
        char_type* pre_buffer{};
        uint32_t const pre_length = static_cast<uint32_t>(test_string.size());

        {
            INFO("Preallocate a string buffer");
            result = xlang_preallocate_string_buffer<char_type>(pre_length, &pre_buffer, &buffer_handle);
            REQUIRE(result == nullptr);
        }

        std::copy(test_string.begin(), test_string.end(), pre_buffer);

        xlang_string str{};
        char_type const* buffer{};
        uint32_t length{};
        {
            INFO("Promote the string buffer and compare its contents");
            result = xlang_promote_string_buffer(buffer_handle, &str, pre_length);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str));

            result = xlang_get_string_raw_buffer(str, &buffer, &length);
            REQUIRE(result == nullptr);
            REQUIRE(test_string == basic_string_view<char_type>{buffer, length});
        }

        xlang_delete_string(str);
    }
}

TEST_CASE("Simple UTF-8 preallocated string buffer")
{
    simple_preallocated<xlang_char8>();
}

TEST_CASE("Simple UTF-16 preallocated string buffer")
{
    simple_preallocated<char16_t>();
}

template <typename char_type>
void convert_string()
{
    using other_type = typename alternate_type<char_type>::type;
    for (size_t i = 0; i < std::size(valid_strings<char_type>::value); ++i)
    {
        auto const test_string = valid_strings<char_type>::value[i];
        xlang_error_info* result{};
        xlang_string str{};
        {
            INFO("Create string");
            result = xlang_create_string(test_string.data(), static_cast<uint32_t>(test_string.size()), &str);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str));
        }

        other_type const* buffer{};
        uint32_t length{};
        {
            INFO("Convert the string");
            result = xlang_get_string_raw_buffer<other_type>(str, &buffer, &length);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<other_type>(str));
            REQUIRE(valid_strings<other_type>::value[i] == basic_string_view<other_type>{buffer, length});
        }

        xlang_delete_string(str);
    }

    for (auto const& test_string : invalid_strings<char_type>::value)
    {
        xlang_error_info* result{};
        xlang_result error_code{};
        xlang_string str{};
        {
            INFO("Create string");
            result = xlang_create_string(test_string.data(), static_cast<uint32_t>(test_string.size()), &str);
            REQUIRE(result == nullptr);
        }

        other_type const* buffer{};
        uint32_t length{};
        {
            INFO("Fail to convert string");
            result = xlang_get_string_raw_buffer<other_type>(str, &buffer, &length);
            REQUIRE(result != nullptr);
            result->GetError(&error_code);
            REQUIRE(error_code == xlang_result::invalid_arg);
            REQUIRE(buffer == nullptr);
            REQUIRE(length == 0);
        }

        xlang_delete_string(str);
    }
}

TEST_CASE("Convert UTF-8 string")
{
    convert_string<xlang_char8>();
}

TEST_CASE("Convert UTF-16 string")
{
    convert_string<char16_t>();
}

template <typename char_type>
void convert_string_reference()
{
    using other_type = typename alternate_type<char_type>::type;
    for (size_t i = 0; i < std::size(valid_strings<char_type>::value); ++i)
    {
        auto const test_string = valid_strings<char_type>::value[i];
        xlang_error_info* result{};
        xlang_string str_ref{};
        xlang_string_header header{};
        {
            INFO("Create string reference");
            result = xlang_create_string_reference<char_type>(test_string.data(), static_cast<uint32_t>(test_string.size()), &header, &str_ref);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str_ref));
        }

        other_type const* buffer_ref{};
        uint32_t length_ref{};
        {
            INFO("Convert the string");
            result = xlang_get_string_raw_buffer<other_type>(str_ref, &buffer_ref, &length_ref);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<other_type>(str_ref));
            REQUIRE(valid_strings<other_type>::value[i] == basic_string_view<other_type>{buffer_ref, length_ref});
        }

        xlang_string str{};
        other_type const* buffer{};
        uint32_t length{};
        {
            INFO("Duplicate string reference and ensure duplicated contents match");
            result = xlang_duplicate_string(str_ref, &str);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<other_type>(str));

            result = xlang_get_string_raw_buffer<other_type>(str, &buffer, &length);
            REQUIRE(result == nullptr);
            REQUIRE(buffer == buffer_ref);
            REQUIRE(length == length_ref);
        }

        xlang_delete_string(str_ref);
        xlang_delete_string(str);
    }

    for (auto const& test_string : invalid_strings<char_type>::value)
    {
        xlang_error_info* result{};
        xlang_result error_code{};
        xlang_string str_ref{};
        xlang_string_header header{};
        {
            INFO("Create string reference");
            result = xlang_create_string_reference<char_type>(test_string.data(), static_cast<uint32_t>(test_string.size()), &header, &str_ref);
            REQUIRE(result == nullptr);
            REQUIRE(has_encoding<char_type>(str_ref));
        }

        other_type const* buffer_ref{};
        uint32_t length_ref{};
        {
            result = xlang_get_string_raw_buffer<other_type>(str_ref, &buffer_ref, &length_ref);
            REQUIRE(result != nullptr);
            result->GetError(&error_code);
            REQUIRE(error_code == xlang_result::invalid_arg);
            REQUIRE(buffer_ref == nullptr);
            REQUIRE(length_ref == 0);
        }

        xlang_delete_string(str_ref);
    }
}

TEST_CASE("Convert UTF-8 string reference")
{
    convert_string_reference<xlang_char8>();
}

TEST_CASE("Convert UTF-16 string reference")
{
    convert_string_reference<char16_t>();
}
