// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::Runtime;
using namespace AppInstaller::Utility;

TEST_CASE("ReadEmptySetting", "[settings]")
{
    std::string name = "nonexistentsetting";

    auto result = GetSettingStream(name);
    REQUIRE(!result);
}

TEST_CASE("SetAndReadSetting", "[settings]")
{
    std::string name = "testsettingname";
    std::string value = "This is the test setting value";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("SetAndReadSettingInContainer", "[settings]")
{
    std::string name = "testcontainer/testsettingname";
    std::string value = "This is the test setting value from inside a container";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("RemoveSetting", "[settings]")
{
    std::string name = "testsettingname";
    std::string value = "This is the test setting value to be removed";

    SetSetting(name, value);

    {
        auto result = GetSettingStream(name);
        REQUIRE(static_cast<bool>(result));

        std::string settingValue = ReadEntireStream(*result);
        REQUIRE(value == settingValue);
    }

    RemoveSetting(name);

    auto result = GetSettingStream(name);
    REQUIRE(!static_cast<bool>(result));
}
