// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Locale.h>

using namespace std::string_view_literals;
using namespace AppInstaller;

TEST_CASE("Locale_SupportedOutputLocales", "[locale]")
{
    // Keep in sync with the `output.locale` enum in schemas/JSON/settings/settings.schema.0.2.json
    // and the localized resource folders under Localization\Resources.
    std::vector<std::string_view> expected =
    {
        "en-US"sv,
        "de-DE"sv,
        "es-ES"sv,
        "fr-FR"sv,
        "it-IT"sv,
        "ja-JP"sv,
        "ko-KR"sv,
        "pt-BR"sv,
        "ru-RU"sv,
        "zh-CN"sv,
        "zh-TW"sv,
    };

    REQUIRE(Locale::GetSupportedOutputLocales() == expected);
}

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
