// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/Registry.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace AppInstaller::Registry;


TEST_CASE("EmptyKey", "[registry]")
{
    Key key;
    REQUIRE(!key);
}

TEST_CASE("Constructor_NotFound", "[registry]")
{
    Key key;
    REQUIRE_THROWS_HR(key = Key(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Foo\\Bar\\Does\\Not\\Exist"), HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
}

TEST_CASE("OpenIfExists_NotFound", "[registry]")
{
    Key key = Key::OpenIfExists(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Foo\\Bar\\Does\\Not\\Exist");
    REQUIRE(!key);
}

TEST_CASE("EnumerateKeys", "[registry]")
{
    Key key{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" };

    for (const auto& subkey : key)
    {
        INFO(subkey.Name());
        Key sk = subkey.Open();
        REQUIRE(sk);
    }
}

TEST_CASE("Values_String", "[registry]")
{
    Key key{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" };

    for (const auto& subkey : key)
    {
        INFO(subkey.Name());
        Key sk = subkey.Open();

        std::optional<Value> value = sk[L"DisplayName"s];
        if (value)
        {
            std::string displayName = value->GetValue<Value::Type::String>();
        }
    }
}

TEST_CASE("Values_ExpandString", "[registry]")
{
    Key key{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" };

    for (const auto& subkey : key)
    {
        INFO(subkey.Name());
        Key sk = subkey.Open();

        std::optional<Value> value = sk[L"UninstallString"s];
        if (value)
        {
            std::string uninstallString = value->GetValue<Value::Type::ExpandString>();
        }
    }
}

TEST_CASE("Values_DWORD", "[registry]")
{
    Key key{ HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" };

    for (const auto& subkey : key)
    {
        INFO(subkey.Name());
        Key sk = subkey.Open();

        std::optional<Value> value = sk[L"VersionMajor"s];
        if (value)
        {
            uint32_t majorVersion = value->GetValue<Value::Type::DWord>();
            // If the endianess is broken, this will fail.
            // It is also very unlikely that any major version would be so large in a real product.
            REQUIRE((majorVersion & 0xFF000000) == 0);
        }
    }
}
