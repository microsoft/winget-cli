// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Locale.h>

using namespace std::string_view_literals;
using namespace AppInstaller;

TEST_CASE("Locale_NormalizeOutputLocale_Supported", "[locale]")
{
    for (const auto& supportedLocale : Locale::GetSupportedOutputLocales())
    {
        auto normalized = Locale::NormalizeOutputLocale(supportedLocale);
        REQUIRE(normalized.has_value());
        REQUIRE(normalized.value() == supportedLocale);
    }
}

TEST_CASE("Locale_NormalizeOutputLocale_CaseInsensitive", "[locale]")
{
    for (const auto& supportedLocale : Locale::GetSupportedOutputLocales())
    {
        auto lower = Locale::NormalizeOutputLocale(Utility::ToLower(supportedLocale));
        REQUIRE(lower.has_value());
        REQUIRE(lower.value() == supportedLocale);

        // Swap the casing of the language and region subtags (e.g. en-US -> EN-us)
        std::string swapped = Utility::ToLower(supportedLocale);
        auto separator = swapped.find('-');
        REQUIRE(separator != std::string::npos);
        for (size_t i = 0; i < separator; ++i)
        {
            swapped[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(swapped[i])));
        }

        auto mixed = Locale::NormalizeOutputLocale(swapped);
        REQUIRE(mixed.has_value());
        REQUIRE(mixed.value() == supportedLocale);
    }
}

TEST_CASE("Locale_NormalizeOutputLocale_Unsupported", "[locale]")
{
    // Not a locale
    REQUIRE(!Locale::NormalizeOutputLocale("not-a-locale"sv));
    REQUIRE(!Locale::NormalizeOutputLocale("en_US.UTF-8"sv));
    REQUIRE(!Locale::NormalizeOutputLocale(""sv));

    // Well formed BCP47 tags that winget does not ship resources for
    REQUIRE(!Locale::NormalizeOutputLocale("en-GB"sv));
    REQUIRE(!Locale::NormalizeOutputLocale("el-GR"sv));
    REQUIRE(!Locale::NormalizeOutputLocale("en"sv));
}
