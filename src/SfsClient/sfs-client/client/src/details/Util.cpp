// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Util.h"

#include <algorithm>
#include <string>

using namespace SFS::details;

bool util::AreEqualI(std::string_view a, std::string_view b)
{
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

bool util::AreNotEqualI(std::string_view a, std::string_view b)
{
    return !AreEqualI(a, b);
}
