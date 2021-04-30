// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <vector>

namespace AppInstaller::Locale
{
    static constexpr double MinimumDistanceScoreAsPerfectMatch = 1.0;
    static constexpr double MinimumDistanceScoreAsCompatibleMatch = 0.9;
    static constexpr double UnknownLanguageDistanceScore = 0.0;

    // Check if a bcp47 language tag is well formed
    bool IsWellFormedBcp47Tag(std::string_view bcp47Tag);

    // Get a score of language distance between target and available. The return value range is 0 to 1.
    // With 1 meaning perfect match and 0 meaning no match.
    double GetDistanceOfLanguage(std::string_view target, std::string_view available);

    // Get the list of user Preferred Languages from settings. Returns an empty vector in rare cases of failure.
    std::vector<std::string> GetUserPreferredLanguages();

    // Get the bcp47 tag from a locale id. Returns empty string if conversion can not be performed.
    std::string LocaleIdToBcp47Tag(LCID localeId);
}