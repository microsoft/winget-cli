// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Regex.h>
#include <AppInstallerStrings.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Regex;


TEST_CASE("Regex_Construction", "[regex]")
{
    Expression empty;
    REQUIRE(!empty);

    Expression lowerVowels("(a|e|i|o|u)");
    REQUIRE(lowerVowels);

    // Ensure functionality to later verify against copy
    REQUIRE(lowerVowels.IsMatch(L"a"));
    REQUIRE(!lowerVowels.IsMatch(L"b"));

    Expression copy = lowerVowels;
    REQUIRE(copy);

    // Ensure that the copy can also work
    REQUIRE(lowerVowels.IsMatch(L"a"));
    REQUIRE(copy.IsMatch(L"a"));

    REQUIRE(!lowerVowels.IsMatch(L"b"));
    REQUIRE(!copy.IsMatch(L"b"));

    Expression moved = std::move(copy);
    REQUIRE(moved);

    REQUIRE(moved.IsMatch(L"a"));
    REQUIRE(!moved.IsMatch(L"b"));
}

TEST_CASE("Regex_IsMatch", "[regex]")
{
    Expression ArchitectureX32{ R"((X32|X86)(?=\P{Nd}|$)(?:\sEDITION)?)", Options::CaseInsensitive };

    REQUIRE(ArchitectureX32.IsMatch(L"X32"));
    REQUIRE(ArchitectureX32.IsMatch(L"X86 edition"));

    REQUIRE(!ArchitectureX32.IsMatch(L"Not a match"));
    REQUIRE(!ArchitectureX32.IsMatch(L"X86 editions"));
}

TEST_CASE("Regex_Replace", "[regex]")
{
    Expression test{ R"((b|d\s|vy))" };
    REQUIRE(test.Replace(L"The bright and swervy", {}) == std::wstring{ L"The right answer" });

    Expression vowels{ "(a|e|i|o|u)", Options::CaseInsensitive };
    REQUIRE(vowels.Replace(L"The QUICK brown fox jumped over the lazy dog.", L"[$0]") == std::wstring{ L"Th[e] Q[U][I]CK br[o]wn f[o]x j[u]mp[e]d [o]v[e]r th[e] l[a]zy d[o]g." });
}

TEST_CASE("Regex_ForEach", "[regex]")
{
    std::wstring input = L"The words in the Sentence but no more";
    std::vector<std::wstring> expected = { L"The", L"words", L"in", L"the", L"Sentence" };

    Expression test{ R"(\S+)" };

    size_t i = 0;
    test.ForEach(input,
        [&](bool isMatch, std::wstring_view text)
        {
            if (!isMatch)
            {
                REQUIRE(text == L" ");
                return true;
            }
            else
            {
                REQUIRE(i < expected.size());
                REQUIRE(text == expected[i]);
                ++i;

                return i < expected.size();
            }
        });
}
