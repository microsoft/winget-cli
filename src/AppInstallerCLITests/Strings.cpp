// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <ExecutionReporter.h>

using namespace std::string_view_literals;
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

TEST_CASE("UTF8ColumnWidth", "[strings]")
{
    REQUIRE(UTF8ColumnWidth("") == 0);
    REQUIRE(UTF8ColumnWidth("a") == 1);
    REQUIRE(UTF8ColumnWidth(" a b c ") == 7);
    REQUIRE(UTF8ColumnWidth("K\xC3\xA4se") == 4); // "Käse"
    REQUIRE(UTF8ColumnWidth("bye\xE2\x80\xA6") == 4); // "bye…"
    REQUIRE(UTF8ColumnWidth("fi\xEF\xAC\x81") == 3); // "fi[fi]" [fi] is not decoupled
    REQUIRE(UTF8ColumnWidth("\xf0\x9f\xa6\x86") == 2); // [duck emoji]
    REQUIRE(UTF8ColumnWidth("\xf0\x9d\x85\xa0\xf0\x9d\x85\xa0") == 2); // [8th note][8th note]
    REQUIRE(UTF8ColumnWidth("\xe6\xb5\x8b\xe8\xaf\x95") == 4); // 测试
    REQUIRE(UTF8ColumnWidth("te\xe6\xb5\x8bs\xe8\xaf\x95t") == 8); // te测s试t
}

TEST_CASE("UTF8TrimRightToColumnWidth", "[strings]")
{
    size_t actualWidth;
    REQUIRE((UTF8TrimRightToColumnWidth("", 0, actualWidth) == "" && actualWidth == 0));
    REQUIRE((UTF8TrimRightToColumnWidth("abcd", 4, actualWidth) == "abcd" && actualWidth == 4));
    REQUIRE((UTF8TrimRightToColumnWidth("abcd", 5, actualWidth) == "abcd" && actualWidth == 4));
    REQUIRE((UTF8TrimRightToColumnWidth("abcd", 2, actualWidth) == "ab" && actualWidth == 2));

    NormalizedString s{ "te\xe6\xb5\x8bs\xe8\xaf\x95t" }; // // te测s试t
    REQUIRE((UTF8TrimRightToColumnWidth(s, 0, actualWidth) == "" && actualWidth == 0));
    REQUIRE((UTF8TrimRightToColumnWidth(s, 2, actualWidth) == "te" && actualWidth == 2));
    REQUIRE((UTF8TrimRightToColumnWidth(s, 3, actualWidth) == "te" && actualWidth == 2));
    REQUIRE((UTF8TrimRightToColumnWidth(s, 4, actualWidth) == "te\xe6\xb5\x8b" && actualWidth == 4));
    REQUIRE((UTF8TrimRightToColumnWidth(s, 8, actualWidth) == "te\xe6\xb5\x8bs\xe8\xaf\x95t" && actualWidth == 8));
    REQUIRE((UTF8TrimRightToColumnWidth(s, 10, actualWidth) == "te\xe6\xb5\x8bs\xe8\xaf\x95t" && actualWidth == 8));
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

TEST_CASE("FoldCase", "[strings]")
{
    REQUIRE(FoldCase(""sv) == FoldCase(""sv));
    REQUIRE(FoldCase("foldcase"sv) == FoldCase("FOLDCASE"sv));
    REQUIRE(FoldCase(u8"f\xF6ldcase"sv) == FoldCase(u8"F\xD6LDCASE"sv));
    REQUIRE(FoldCase(u8"foldc\x430se"sv) == FoldCase(u8"FOLDC\x410SE"sv));
}

TEST_CASE("ExpandEnvironmentVariables", "[strings]")
{
    wchar_t buffer[MAX_PATH];
    GetTempPathW(ARRAYSIZE(buffer), buffer);

    std::wstring tempPath = buffer;
    if (!tempPath.empty() && tempPath.back() == '\\')
    {
        tempPath.resize(tempPath.size() - 1);
    }

    REQUIRE(ExpandEnvironmentVariables(L"%TEMP%") == tempPath);
}

TEST_CASE("PathOutput", "[strings]")
{
    std::string original = "\xe6\xb5\x8b\xe8\xaf\x95";
    std::filesystem::path path = ConvertToUTF16(original);
    AICLI_LOG(Test, Info, << path);

    std::istringstream in;
    std::ostringstream out;
    AppInstaller::CLI::Execution::Reporter reporter{ out, in };

    reporter.Info() << path;

    std::string output = out.str();
    REQUIRE(output.substr(output.size() - original.size()) == original);
}

TEST_CASE("ReplaceWhileCopying", "[strings]")
{
    REQUIRE(ReplaceWhileCopying(L"A red apple", L"red", L"green") == L"A green apple");
    REQUIRE(ReplaceWhileCopying(L"A red, red apple", L"red", L"green") == L"A green, green apple");
    REQUIRE(ReplaceWhileCopying(L"A red, red apple", L"ed", L"ad") == L"A rad, rad apple");
    REQUIRE(ReplaceWhileCopying(L"A red apple", L"p", L"f") == L"A red affle");
    REQUIRE(ReplaceWhileCopying(L"A red apple", L"", L"green") == L"A red apple");
}
