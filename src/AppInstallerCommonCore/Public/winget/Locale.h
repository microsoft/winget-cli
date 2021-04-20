// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string>
#include <vector>

namespace AppInstaller::Utility
{
    // Check if a bcp47 language tag is well formed
    bool IsWellFormedBcp47Tag(const std::string& bcp47Tag);

    // Get a score of language distance between target and available. The return value range is 0 to 1.
    // With 1 meaning perfect match and 0 meaning no match.
    double GetDistanceOfLanguage(const std::string& target, const std::string& available);

    // Get the list of user Preferred Languages from settings. Returns an empty vector in rare cases of failure.
    std::vector<std::string> GetUserPreferredLanguages();
}