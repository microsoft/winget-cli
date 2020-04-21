// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>

using namespace AppInstaller::Utility;


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
