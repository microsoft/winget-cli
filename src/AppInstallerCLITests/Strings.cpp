// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include <ExecutionReporter.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

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

    // Embedded null
    std::string_view input3{ "Test\0Case", 9 };
    REQUIRE(NormalizedString(input3) == "Test Case");
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

TEST_CASE("MakeSuitablePathPart", "[strings]")
{
    REQUIRE(MakeSuitablePathPart("A\\B") == "A_B");
    REQUIRE(MakeSuitablePathPart("A\\B/") == "A_B_");
    REQUIRE(MakeSuitablePathPart("*AB") == "_AB");
    REQUIRE(MakeSuitablePathPart(u8"f*\xF6*ldcase") == u8"f_\xF6_ldcase");
    REQUIRE(MakeSuitablePathPart(".") == "_");
    REQUIRE(MakeSuitablePathPart("..") == "._");
    REQUIRE(MakeSuitablePathPart(std::string(300, ' ')) == SHA256::ConvertToString(SHA256::ComputeHash(std::string(300, ' '))));
    REQUIRE_THROWS_HR(MakeSuitablePathPart("COM1"), E_INVALIDARG);
    REQUIRE_THROWS_HR(MakeSuitablePathPart("NUL.txt"), E_INVALIDARG);
}

TEST_CASE("GetFileNameFromURI", "[strings]")
{
    REQUIRE(GetFileNameFromURI("https://github.com/microsoft/winget-cli/pull/1722").u8string() == "1722");
    REQUIRE(GetFileNameFromURI("https://github.com/microsoft/winget-cli/README.md").u8string() == "README.md");
    REQUIRE(GetFileNameFromURI("https://microsoft.com/").u8string() == "");
}

void ValidateSplitFileName(std::string_view uri, std::string_view base, std::string_view fileName)
{
    auto split = SplitFileNameFromURI(uri);
    REQUIRE(split.first == base);
    REQUIRE(split.second.u8string() == fileName);
}

TEST_CASE("SplitFileNameFromURI", "[strings]")
{
    ValidateSplitFileName("https://github.com/microsoft/winget-cli/pull/1722", "https://github.com/microsoft/winget-cli/pull/", "1722");
    ValidateSplitFileName("https://github.com/microsoft/winget-cli/README.md", "https://github.com/microsoft/winget-cli/", "README.md");
    ValidateSplitFileName("https://microsoft.com/", "https://microsoft.com/", "");
}

TEST_CASE("SplitIntoWords", "[strings]")
{
    REQUIRE(SplitIntoWords("A B") == std::vector<std::string>{ "A", "B" });
    REQUIRE(SplitIntoWords("Some-Thing") == std::vector<std::string>{ "Some", "Thing" });

    // 私のテスト = "My test" according to an online translator
    // Split as "私" "の" "テスト"
    REQUIRE(SplitIntoWords("\xe7\xa7\x81\xe3\x81\xae\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88") == std::vector<std::string>{ "\xe7\xa7\x81", "\xe3\x81\xae", "\xe3\x83\x86\xe3\x82\xb9\xe3\x83\x88" });
}

TEST_CASE("ReplaceEmbeddedNullCharacters", "[strings]")
{
    std::string test = "Test Parts";
    test[4] = '\0';
    ReplaceEmbeddedNullCharacters(test);
    REQUIRE(test == "Test Parts");
}

TEST_CASE("HexStrings", "[strings]")
{
    std::vector<uint8_t> buffer{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    std::string value = "000102030405060708090a0b0c0d0e0f";

    REQUIRE(value == ConvertToHexString(buffer));
    REQUIRE(std::equal(buffer.begin(), buffer.end(), ParseFromHexString(value).begin()));
}

TEST_CASE("Join", "[strings]")
{
    std::vector<LocIndString> list_0{ };
    std::vector<LocIndString> list_1{ "A"_lis };
    std::vector<LocIndString> list_2{ "A"_lis, "B"_lis };

    REQUIRE(""_lis == Join(", "_liv, list_0));
    REQUIRE("A"_lis == Join(", "_liv, list_1));
    REQUIRE("A, B"_lis == Join(", "_liv, list_2));
    REQUIRE("AB"_lis == Join(""_liv, list_2));
}

TEST_CASE("Format", "[strings]")
{
    REQUIRE("First Second" == Format("{0} {1}", "First", "Second"));
    REQUIRE("First Second" == Format("{1} {0}", "Second", "First"));
    REQUIRE("First Second" == Format("{0} {1}", "First", "Second", "(Extra", "Input", "Ignored)"));
    REQUIRE("First Second First Second" == Format("{0} {1} {0} {1}", "First", "Second"));

    // Note: C++20 std::format will throw an exception for this test case
    REQUIRE("First {1}" == Format("{0} {1}", "First"));
}

TEST_CASE("SplitIntoLines", "[strings]")
{
    REQUIRE(SplitIntoLines("Boring test") == std::vector<std::string>{ "Boring test" });
    REQUIRE(SplitIntoLines(
        "I'm Luffy! The Man Who Will Become the Pirate King!\r-Monkey D. Luffy") == std::vector<std::string>{ "I'm Luffy! The Man Who Will Become the Pirate King!", "-Monkey D. Luffy" });
    REQUIRE(SplitIntoLines(
        "I want live!\n-Nico Robin") == std::vector<std::string>{ "I want live!", "-Nico Robin" });
    REQUIRE(SplitIntoLines(
        "You want my treasure?\rYou can have it!\nI left everything I gathered in one place!\r\nYou just have to find it!")
        == std::vector<std::string>{ "You want my treasure?", "You can have it!", "I left everything I gathered in one place!", "You just have to find it!" });
}

TEST_CASE("SplitWithSeparator", "[strings]")
{
    std::vector<std::string> test1 = Split("first;second;third", ';');
    REQUIRE(test1.size() == 3);
    REQUIRE(test1[0] == "first");
    REQUIRE(test1[1] == "second");
    REQUIRE(test1[2] == "third");

    std::vector<std::string> test2 = Split("two  spaces", ' ');
    REQUIRE(test2.size() == 3);
    REQUIRE(test2[0] == "two");
    REQUIRE(test2[1] == "");
    REQUIRE(test2[2] == "spaces");

    std::vector<std::string> test3 = Split("test", '.');
    REQUIRE(test3.size() == 1);
    REQUIRE(test3[0] == "test");

    std::vector<std::string> test4 = Split(" trim |    spaces ", '|', true);
    REQUIRE(test4.size() == 2);
    REQUIRE(test4[0] == "trim");
    REQUIRE(test4[1] == "spaces");
}

TEST_CASE("ConvertGuid", "[strings]")
{
    std::string validGuidString = "{4d1e55b2-f16f-11cf-88cb-001111000030}";
    GUID guid = { 0x4d1e55b2, 0xf16f, 0x11cf, 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 };

    REQUIRE(CaseInsensitiveEquals(ConvertGuidToString(guid), validGuidString));
}

TEST_CASE("FindControlCodeToConvert", "[strings]")
{
    REQUIRE(FindControlCodeToConvert("No codes") == std::string::npos);
    REQUIRE(FindControlCodeToConvert("Allowed codes: \t\r\n") == std::string::npos);
    REQUIRE(FindControlCodeToConvert("\x1bSkipped code", 1) == std::string::npos);

    REQUIRE(FindControlCodeToConvert("\x1bUnskipped code") == 0);
    REQUIRE(FindControlCodeToConvert("Escape code: \x1b") == 13);

    std::string_view allCodes{ "\x0\x1\x2\x3\x4\x5\x6\x7\x8\xb\xc\xe\xf\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x7f"sv };
    for (size_t i = 0; i < allCodes.length(); ++i)
    {
        REQUIRE(FindControlCodeToConvert(allCodes, i) == i);
    }
}

TEST_CASE("ConvertControlCodesToPictures", "[strings]")
{
    REQUIRE(ConvertControlCodesToPictures("No codes") == "No codes");
    REQUIRE(ConvertControlCodesToPictures("Allowed codes: \t\r\n") == "Allowed codes: \t\r\n");

    REQUIRE(ConvertControlCodesToPictures("\x1b Code First") == ConvertToUTF8(L"\x241b Code First"));
    REQUIRE(ConvertControlCodesToPictures("Escape code: \x1b") == ConvertToUTF8(L"Escape code: \x241b"));

    std::string_view allCodes{ "\x0\x1\x2\x3\x4\x5\x6\x7\x8\xb\xc\xe\xf\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x7f"sv };
    std::wstring_view allPictures{ L"\x2400\x2401\x2402\x2403\x2404\x2405\x2406\x2407\x2408\x240b\x240c\x240e\x240f\x2410\x2411\x2412\x2413\x2414\x2415\x2416\x2417\x2418\x2419\x241a\x241b\x241c\x241d\x241e\x241f\x2421"sv };

    REQUIRE(ConvertControlCodesToPictures(allCodes) == ConvertToUTF8(allPictures));
}
