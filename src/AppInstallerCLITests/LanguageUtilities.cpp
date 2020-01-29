// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerLanguageUtilities.h>

using namespace AppInstaller;


TEST_CASE("DestructionToken", "[langutil]")
{
    DestructionToken beginToken = true;
    DestructionToken endToken = false;

    REQUIRE(beginToken);
    REQUIRE(!endToken);

    endToken = std::move(beginToken);

    REQUIRE(!beginToken);
    REQUIRE(endToken);
}

TEST_CASE("ZipIterator", "[langutil]")
{
    std::vector<int> ints{ 1, 2, 3 };
    std::vector<std::string> strings{ "a", "b", "c" };

    REQUIRE(ints.size() == strings.size());

    std::vector<std::tuple<int, std::string>> zipped;

    for (auto values : ZipIterator(ints, strings))
    {
        zipped.emplace_back(values);
    }

    REQUIRE(ints.size() == zipped.size());

    for (size_t i = 0; i < zipped.size(); ++i)
    {
        REQUIRE(ints[i] == std::get<0>(zipped[i]));
        REQUIRE(strings[i] == std::get<1>(zipped[i]));
    }
}
