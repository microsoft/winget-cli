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

    auto result = Stream{ name }.Get();
    REQUIRE(!result);
}

TEST_CASE("SetAndReadSetting", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testsettingname" };
    std::string value = "This is the test setting value";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("SetAndReadSettingInContainer", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testcontainer/testsettingname" };
    std::string value = "This is the test setting value from inside a container";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("RemoveSetting", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testsettingname" };
    std::string value = "This is the test setting value to be removed";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    {
        auto result = stream.Get();
        REQUIRE(static_cast<bool>(result));

        std::string settingValue = ReadEntireStream(*result);
        REQUIRE(value == settingValue);
    }

    stream.Remove();

    auto result = stream.Get();
    REQUIRE(!static_cast<bool>(result));
}

TEST_CASE("SetAndReadUserFileSetting", "[settings]")
{
    StreamDefinition name{ Type::UserFile, "userfilesetting" };
    std::string value = "This is the test setting value for a user file";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("ReadEmptySecureSetting", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_nonexistentsetting" };

    auto result = Stream{ name }.Get();
    REQUIRE(!result);
}

TEST_CASE("SetAndReadSecureSetting", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("SetAndReadSecureSettingInContainer", "[settings]")
{
    StreamDefinition name{ Type::Secure, "testcontainer/secure_testsettingname" };
    std::string value = "This is the test setting value from inside a container";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);
}

TEST_CASE("RemoveSecureSetting", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value to be removed";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    {
        auto result = stream.Get();
        REQUIRE(static_cast<bool>(result));

        std::string settingValue = ReadEntireStream(*result);
        REQUIRE(value == settingValue);
    }

    stream.Remove();

    auto result = stream.Get();
    REQUIRE(!static_cast<bool>(result));
}

TEST_CASE("SetAndReadSecureSetting_SecureDataRemoved", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);

    std::filesystem::remove(GetPathTo(PathName::SecureSettings) / name.Name);

    REQUIRE_THROWS_HR(stream.Get(), SPAPI_E_FILE_HASH_NOT_IN_CATALOG);
}

TEST_CASE("SetAndReadSecureSetting_DataTampered", "[settings]")
{
    StreamDefinition name{ Type::Secure, "secure_testsettingname" };
    std::string value = "This is the test setting value";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);

    StreamDefinition insecureName = name;
    insecureName.Type = Type::Standard;

    REQUIRE(Stream{ insecureName }.Set("Tampered data"));

    REQUIRE_THROWS_HR(stream.Get(), HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR));
}
