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
    StreamDefinition name{ Type::Standard, "nonexistentsetting" };

    auto result = GetSettingStream(name);
    REQUIRE(!result);
}

TEST_CASE("SetAndReadSetting", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testsettingname" };
    std::string value = "This is the test setting value";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("SetAndReadSettingInContainer", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testcontainer/testsettingname" };
    std::string value = "This is the test setting value from inside a container";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("RemoveSetting", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testsettingname" };
    std::string value = "This is the test setting value to be removed";

    SetSetting(name, value);

    {
        auto result = GetSettingStream(name);
        REQUIRE(static_cast<bool>(result));

        std::string settingValue = ReadEntireStream(*result);
        REQUIRE(value == settingValue);
    }

    RemoveSetting( name);

    auto result = GetSettingStream(name);
    REQUIRE(!static_cast<bool>(result));
}

TEST_CASE("SetAndReadUserFileSetting", "[settings]")
{
    StreamDefinition name{ Type::UserFile, "userfilesetting" };
    std::string value = "This is the test setting value for a user file";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("ReadEmptySecureSetting", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_nonexistentsetting" };

    auto result = GetSettingStream(name);
    REQUIRE(!result);
}

TEST_CASE("SetAndReadSecureSetting", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("SetAndReadSecureSettingInContainer", "[settings]")
{
    StreamDefinition name{ Type::Secure, "testcontainer/secure_testsettingname" };
    std::string value = "This is the test setting value from inside a container";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("RemoveSecureSetting", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
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

TEST_CASE("SetAndReadSecureSetting_SecureDataRemoved", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);

    std::filesystem::remove(GetPathTo(PathName::SecureSettings) / name.Path);

    REQUIRE_THROWS_HR(GetSettingStream(name), SPAPI_E_FILE_HASH_NOT_IN_CATALOG);
}

TEST_CASE("SetAndReadSecureSetting_DataTampered", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value";

    SetSetting(name, value);

    auto result = GetSettingStream(name);
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);

    StreamDefinition insecureName = name;
    insecureName.Type = Type::Standard;

    SetSetting(insecureName, "Tampered data");

    REQUIRE_THROWS_HR(GetSettingStream(name), HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR));
}
