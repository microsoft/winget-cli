// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerStrings.h>
#include <winget/Registry.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace AppInstaller::Registry;
using namespace AppInstaller::Utility;
using namespace TestCommon;

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
    wil::unique_hkey root = RegCreateVolatileTestRoot();

    std::vector<std::wstring> subKeyNames = { L"A", L"BEE", L"SEE", L"deigh" };
    for (const auto& name : subKeyNames)
    {
        RegCreateVolatileSubKey(root.get(), name);
    }

    Key key{ root.get(), L"" };

    for (const auto& subkey : key)
    {
        INFO(subkey.Name());

        std::wstring nameUtf16 = ConvertToUTF16(subkey.Name());

        auto itr = std::find(subKeyNames.begin(), subKeyNames.end(), nameUtf16);
        if (itr == subKeyNames.end())
        {
            FAIL();
        }
        else
        {
            subKeyNames.erase(itr);
        }

        Key sk = subkey.Open();
        REQUIRE(sk);
    }

    REQUIRE(subKeyNames.empty());
}

TEST_CASE("Values_String", "[registry]")
{
    std::wstring valueName = L"TestValueName";
    std::wstring valueValue = L"TestValueValue";

    wil::unique_hkey root = RegCreateVolatileTestRoot();
    SetRegistryValue(root.get(), valueName, valueValue);

    Key key{ root.get(), L"" };

    auto value = key[valueName];
    REQUIRE(value);
    REQUIRE(value->GetType() == Value::Type::String);
    REQUIRE(value->GetValue<Value::Type::String>() == ConvertToUTF8(valueValue));
}

TEST_CASE("Values_WideStringWithNarrowNull", "[registry]")
{
    std::wstring valueName = L"TestValueName";
    std::wstring valueValue = L"TestValueValue";

    wil::unique_hkey root = RegCreateVolatileTestRoot();

    // Copy the bytes from the string value into a byte vector
    std::vector<BYTE> valueBytes;
    valueBytes.resize((valueValue.length() + 1) * sizeof(wchar_t));
    memcpy_s(valueBytes.data(), valueBytes.size(), valueValue.c_str(), (valueValue.length() + 1) * sizeof(wchar_t));
    // Remove the last byte to make a narrow null
    valueBytes.resize(valueBytes.size() - 1);

    SetRegistryValue(root.get(), valueName, valueBytes, REG_SZ);

    Key key{ root.get(), L"" };

    auto value = key[valueName];
    REQUIRE(value);
    REQUIRE(value->GetType() == Value::Type::String);
    REQUIRE(value->GetValue<Value::Type::String>() == ConvertToUTF8(valueValue));
}

TEST_CASE("Values_ExpandString", "[registry]")
{
    std::wstring valueName = L"TestValueName";
    std::wstring valueValue = L"%TEMP%";

    wil::unique_hkey root = RegCreateVolatileTestRoot();
    SetRegistryValue(root.get(), valueName, valueValue, REG_EXPAND_SZ);

    Key key{ root.get(), L"" };

    auto value = key[valueName];
    REQUIRE(value);
    REQUIRE(value->GetType() == Value::Type::ExpandString);
    REQUIRE(value->GetValue<Value::Type::String>() == ConvertToUTF8(valueValue));

    wchar_t buffer[MAX_PATH];
    GetTempPathW(ARRAYSIZE(buffer), buffer);

    std::string tempPath = ConvertToUTF8(buffer);
    if (!tempPath.empty() && tempPath.back() == '\\')
    {
        tempPath.resize(tempPath.size() - 1);
    }

    REQUIRE(value->GetValue<Value::Type::ExpandString>() == tempPath);
}

TEST_CASE("Values_Binary", "[registry]")
{
    std::wstring valueName = L"TestValueName";
    std::vector<BYTE> valueValue = { 2, 7, 3, 14, 42 };

    wil::unique_hkey root = RegCreateVolatileTestRoot();
    SetRegistryValue(root.get(), valueName, valueValue);

    Key key{ root.get(), L"" };

    auto value = key[valueName];
    REQUIRE(value);
    REQUIRE(value->GetType() == Value::Type::Binary);

    auto result = value->GetValue<Value::Type::Binary>();
    REQUIRE(result.size() == valueValue.size());
    for (size_t i = 0; i < result.size(); ++i)
    {
        INFO(i);
        REQUIRE(result[i] == valueValue[i]);
    }
}

TEST_CASE("Values_DWORD", "[registry]")
{
    std::wstring valueName = L"TestValueName";
    DWORD valueValue = 42;

    wil::unique_hkey root = RegCreateVolatileTestRoot();
    SetRegistryValue(root.get(), valueName, valueValue);

    Key key{ root.get(), L"" };

    auto value = key[valueName];
    REQUIRE(value);
    REQUIRE(value->GetType() == Value::Type::DWord);
    REQUIRE(value->GetValue<Value::Type::DWord>() == valueValue);
}
