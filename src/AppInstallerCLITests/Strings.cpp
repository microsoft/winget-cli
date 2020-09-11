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
    REQUIRE(UTF8Substring("abcd", 0, 5) == "abcd");
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

TEST_CASE("UTF8TerminalLength", "[strings]")
{
    REQUIRE(UTF8TerminalLength("") == 0);
    REQUIRE(UTF8TerminalLength("a") == 1);
    REQUIRE(UTF8TerminalLength(" a b c ") == 7);
    REQUIRE(UTF8TerminalLength("K\xC3\xA4se") == 4); // "Käse"
    REQUIRE(UTF8TerminalLength("bye\xE2\x80\xA6") == 4); // "bye…"
    REQUIRE(UTF8TerminalLength("\xf0\x9f\xa6\x86") == 2); // [duck emoji]
    REQUIRE(UTF8TerminalLength("\xf0\x9d\x85\xa0\xf0\x9d\x85\xa0") == 2); // [8th note][8th note]
    REQUIRE(UTF8TerminalLength("\xe6\xb5\x8b\xe8\xaf\x95") == 4); // 测试
    REQUIRE(UTF8TerminalLength("te\xe6\xb5\x8bs\xe8\xaf\x95t") == 8); // te测s试t
}

TEST_CASE("UTF8TrimRightToTerminalLength", "[strings]")
{
    REQUIRE(UTF8TrimRightToTerminalLength("", 0) == "");
    REQUIRE(UTF8TrimRightToTerminalLength("abcd", 4) == "abcd");
    REQUIRE(UTF8TrimRightToTerminalLength("abcd", 5) == "abcd");
    REQUIRE(UTF8TrimRightToTerminalLength("abcd", 2) == "ab");

    const char* s = "te\xe6\xb5\x8bs\xe8\xaf\x95t"; // // te测s试t
    REQUIRE(UTF8TrimRightToTerminalLength(s, 0) == "");
    REQUIRE(UTF8TrimRightToTerminalLength(s, 2) == "te");
    REQUIRE(UTF8TrimRightToTerminalLength(s, 3) == "te");
    REQUIRE(UTF8TrimRightToTerminalLength(s, 4) == "te\xe6\xb5\x8b");
    REQUIRE(UTF8TrimRightToTerminalLength(s, 8) == "te\xe6\xb5\x8bs\xe8\xaf\x95t");
    REQUIRE(UTF8TrimRightToTerminalLength(s, 10) == "te\xe6\xb5\x8bs\xe8\xaf\x95t");
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

TEST_CASE("CaseInsensitiveStartsWith", "[strings]")
{
    REQUIRE(CaseInsensitiveStartsWith("startswith", "starts"));
    REQUIRE(CaseInsensitiveStartsWith("startswith", "STAR"));
    REQUIRE(CaseInsensitiveStartsWith("startswith", "startSWITH"));
    REQUIRE(CaseInsensitiveStartsWith("startswith", ""));

    REQUIRE(!CaseInsensitiveStartsWith("starts", "startswith"));
    REQUIRE(!CaseInsensitiveStartsWith("", "nuffing"));
    REQUIRE(!CaseInsensitiveStartsWith("withstarts", "starts"));
    REQUIRE(!CaseInsensitiveStartsWith(" starts", "starts"));
}
