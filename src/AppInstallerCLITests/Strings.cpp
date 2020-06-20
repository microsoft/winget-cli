// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>

using namespace AppInstaller::Utility;


TEST_CASE("UTF8Length", "[strings]")
{
    REQUIRE(UTF8Length("") == 0);
    REQUIRE(UTF8Length("a") == 1);
    REQUIRE(UTF8Length(" a b c ") == 7);
    REQUIRE(UTF8Length("K\xC3\xA4se") == 4); // "Käse"
    REQUIRE(UTF8Length("bye\xE2\x80\xA6") == 4); // "bye…"
    REQUIRE(UTF8Length("\xf0\x9f\xa6\x86") == 1); // [duck emoji]
    REQUIRE(UTF8Length("\xf0\x9d\x85\xa0\xf0\x9d\x85\xa0") == 2); // [8th note][8th note]
}

TEST_CASE("UTF8Substring", "[strings]")
{    
    REQUIRE(UTF8Substring("", 0, 0) == "");
    REQUIRE(UTF8Substring("abcd", 0, 4) == "abcd");
    REQUIRE(UTF8Substring("abcd", 0, 2) == "ab");
    REQUIRE(UTF8Substring("abcd", 1, 0) == "");
    REQUIRE(UTF8Substring("abcd", 1, 1) == "b");
    REQUIRE(UTF8Substring("abcd", 1, 3) == "bcd");
    REQUIRE(UTF8Substring("abcd", 4, 0) == "");

    const char* s = "\xf0\x9f\xa6\x86s like \xf0\x9f\x8c\x8a"; // [duck emoji]s like [wave emoji]
    REQUIRE(UTF8Substring(s, 0, 9) == "\xf0\x9f\xa6\x86s like \xf0\x9f\x8c\x8a");
    REQUIRE(UTF8Substring(s, 0, 1) == "\xf0\x9f\xa6\x86");
    REQUIRE(UTF8Substring(s, 0, 2) == "\xf0\x9f\xa6\x86s");
    REQUIRE(UTF8Substring(s, 1, 7) == "s like ");
    REQUIRE(UTF8Substring(s, 1, 8) == "s like \xf0\x9f\x8c\x8a");
}

TEST_CASE("Normalize", "[strings]")
{
    REQUIRE(Normalize("test") == "test");

    // A + combining Dieresis => single A with umlaut char
    REQUIRE(Normalize(L"\x41\x308") == L"\xC4");
    // This will stop working in C++20, sigh.
    REQUIRE(Normalize(u8"\x41\x308") == u8"\xC4");

    // Ligature fi => f + i
    REQUIRE(Normalize(u8"\xFB01") == u8"fi");
}

TEST_CASE("NormalizedString", "[strings]")
{
    REQUIRE(NormalizedString("test") == "test");
    std::string input = "test";
    REQUIRE(NormalizedString(input) == input);

    // A + combining Dieresis => single A with umlaut char
    REQUIRE(NormalizedString(std::wstring_view(L"\x41\x308")) == u8"\xC4");
    // This will stop working in C++20, sigh.
    input = u8"\x41\x308";
    REQUIRE(NormalizedString(input) == u8"\xC4");

    // Ligature fi => f + i
    std::string_view input2 = u8"\xFB01";
    REQUIRE(NormalizedString(input2) == u8"fi");
}

TEST_CASE("Trim", "[strings]")
{
    std::string str;
    REQUIRE(Trim(str.assign("")) == "");
    REQUIRE(Trim(str.assign(" ")) == "");
    REQUIRE(Trim(str.assign(" \t ")) == "");
    REQUIRE(Trim(str.assign(" a")) == "a");
    REQUIRE(Trim(str.assign("bght ")) == "bght");
    REQUIRE(Trim(str.assign("\tStuff\f")) == "Stuff");
    REQUIRE(Trim(str.assign("Multiple words")) == "Multiple words");
    REQUIRE(Trim(str.assign("         Multiple words")) == "Multiple words");
    REQUIRE(Trim(str.assign("Much after is taken \f\n\r\t\v\v\t\r\n\f ")) == "Much after is taken");
}
