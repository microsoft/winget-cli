// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/Settings.h>

using namespace AppInstaller::Runtime;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

TEST_CASE("ReadEmptySetting", "[settings]")
{
    std::string name = "nonexistentsetting";

    auto result = GetSettingStream(Type::Standard, name);
    REQUIRE(!result);
}

TEST_CASE("SetAndReadSetting", "[settings]")
{
    std::string name = "testsettingname";
    std::string value = "This is the test setting value";

    SetSetting(Type::Standard, name, value);

    auto result = GetSettingStream(Type::Standard, name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("SetAndReadSettingInContainer", "[settings]")
{
    std::string name = "testcontainer/testsettingname";
    std::string value = "This is the test setting value from inside a container";

    SetSetting(Type::Standard, name, value);

    auto result = GetSettingStream(Type::Standard, name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("RemoveSetting", "[settings]")
{
    std::string name = "testsettingname";
    std::string value = "This is the test setting value to be removed";

    SetSetting(Type::Standard, name, value);

    {
        auto result = GetSettingStream(Type::Standard, name);
        REQUIRE(static_cast<bool>(result));

        std::string settingValue = ReadEntireStream(*result);
        REQUIRE(value == settingValue);
    }

    RemoveSetting(Type::Standard, name);

    auto result = GetSettingStream(Type::Standard, name);
    REQUIRE(!static_cast<bool>(result));
}

TEST_CASE("SetAndReadUserFileSetting", "[settings]")
{
    std::string name = "userfilesetting";
    std::string value = "This is the test setting value for a user file";

    SetSetting(Type::UserFile, name, value);

    auto result = GetSettingStream(Type::UserFile, name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}
