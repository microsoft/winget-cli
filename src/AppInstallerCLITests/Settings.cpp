// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"

#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/Settings.h>
#include <iterator>

using namespace AppInstaller::Runtime;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

namespace
{
    struct AdminFileSettingsTest
    {
        TestCommon::TempDirectory Directory{ "AdminFileSettings", false };

        AdminFileSettingsTest() :
            m_originalRead(GetPathDetailsFor(PathName::SecureSettingsForRead)),
            m_originalWrite(GetPathDetailsFor(PathName::SecureSettingsForWrite))
        {
            AppInstaller::Filesystem::PathDetails read;
            read.Path = Directory;
            read.Create = false;
            TestHook_SetPathOverride(PathName::SecureSettingsForRead, read);

            auto write = read;
            write.Create = true;
            write.SetOwner(AppInstaller::Filesystem::ACEPrincipal::Admins);
            if (!IsRunningAsSystem())
            {
                write.ACL[AppInstaller::Filesystem::ACEPrincipal::CurrentUser] = AppInstaller::Filesystem::ACEPermissions::ReadExecute;
            }
            write.ACL[AppInstaller::Filesystem::ACEPrincipal::System] = AppInstaller::Filesystem::ACEPermissions::All;
            TestHook_SetPathOverride(PathName::SecureSettingsForWrite, write);
        }

        ~AdminFileSettingsTest()
        {
            TestHook_SetPathOverride(PathName::SecureSettingsForRead, m_originalRead);
            TestHook_SetPathOverride(PathName::SecureSettingsForWrite, m_originalWrite);
        }

    private:
        AppInstaller::Filesystem::PathDetails m_originalRead;
        AppInstaller::Filesystem::PathDetails m_originalWrite;
    };

    wil::unique_handle CreateNonAdminToken()
    {
        wil::unique_handle processToken;
        THROW_IF_WIN32_BOOL_FALSE(OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &processToken));

        auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
        SID_AND_ATTRIBUTES disabledGroup{ adminSID.get(), 0 };
        wil::unique_handle restrictedToken;
        THROW_IF_WIN32_BOOL_FALSE(CreateRestrictedToken(processToken.get(), DISABLE_MAX_PRIVILEGE, 1, &disabledGroup, 0, nullptr, 0, nullptr, &restrictedToken));
        return restrictedToken;
    }
}

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

TEST_CASE("AdminFileSettings_ReadDoesNotCreateDirectory", "[settings]")
{
    AdminFileSettingsTest test;
    if (!IsRunningAsAdminOrSystem())
    {
        TestCommon::UserSettingsTest settings{ "{}" };
        REQUIRE_NOTHROW(settings.PrepareToShellExecuteFile(true));
        REQUIRE_THROWS_HR(UserSettings::CreateAdminSettingsFile(), E_ACCESSDENIED);
        REQUIRE_FALSE(std::filesystem::exists(test.Directory));
    }

    for (const auto& definition : { Stream::PrimaryAdminSettingsFile, Stream::BackupAdminSettingsFile })
    {
        Stream stream{ definition };
        REQUIRE(stream.GetPath() == test.Directory.GetPath() / definition.Name);
        REQUIRE_FALSE(stream.Get());
        REQUIRE_FALSE(std::filesystem::exists(test.Directory));

        if (IsRunningAsAdminOrSystem())
        {
            stream.Remove();
            REQUIRE_FALSE(std::filesystem::exists(test.Directory));
        }
    }
}

TEST_CASE("AdminFileSettings_PlainFileStorage", "[settings]")
{
    if (!IsRunningAsAdminOrSystem())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    AdminFileSettingsTest test;
    const std::string value = "{\"value\":\"" + std::string(9000, 'x') + "\"}";
    const std::string editedValue = "{\"value\":\"edited\"}";

    for (const auto& definition : { Stream::PrimaryAdminSettingsFile, Stream::BackupAdminSettingsFile })
    {
        Stream stream{ definition };
        REQUIRE(stream.Set(value));

        auto result = stream.Get();
        REQUIRE(result);
        REQUIRE(ReadEntireStream(*result) == value);
        result.reset();

        {
            std::ifstream file{ stream.GetPath(), std::ios::binary };
            REQUIRE(ReadEntireStream(file) == value);
        }

        {
            std::ofstream file{ stream.GetPath(), std::ios::binary | std::ios::trunc };
            file << editedValue;
            REQUIRE(file.good());
        }

        result = stream.Get();
        REQUIRE(result);
        REQUIRE(ReadEntireStream(*result) == editedValue);
        result.reset();
    }

    REQUIRE_FALSE(GetPathDetailsFor(PathName::SecureSettingsForWrite).ShouldApplyACL());
    REQUIRE(std::distance(std::filesystem::directory_iterator(test.Directory), std::filesystem::directory_iterator{}) == 2);

    Stream{ Stream::PrimaryAdminSettingsFile }.Remove();
    REQUIRE_FALSE(Stream{ Stream::PrimaryAdminSettingsFile }.Get());
    REQUIRE(Stream{ Stream::BackupAdminSettingsFile }.Get());

    Stream{ Stream::BackupAdminSettingsFile }.Remove();
    REQUIRE_FALSE(Stream{ Stream::BackupAdminSettingsFile }.Get());
}

TEST_CASE("AdminFileSettings_MutationsRequireAdmin", "[settings]")
{
    if (IsRunningAsSystem())
    {
        WARN("Test requires a non-SYSTEM user. Skipped.");
        return;
    }

    AdminFileSettingsTest test;
    std::filesystem::create_directories(test.Directory);
    Stream primary{ Stream::PrimaryAdminSettingsFile };
    const std::string value = "{\"value\":\"original\"}";
    {
        std::ofstream file{ primary.GetPath(), std::ios::binary };
        file << value;
        REQUIRE(file.good());
    }

    auto token = CreateNonAdminToken();
    THROW_IF_WIN32_BOOL_FALSE(ImpersonateLoggedOnUser(token.get()));
    auto revert = wil::scope_exit([]() { FAIL_FAST_IF_WIN32_BOOL_FALSE(RevertToSelf()); });
    REQUIRE_FALSE(IsRunningAsAdminOrSystem());

    REQUIRE_THROWS_HR(UserSettings::CreateAdminSettingsFile(), E_ACCESSDENIED);
    TestCommon::UserSettingsTest settings{ "{}" };
    REQUIRE_NOTHROW(settings.PrepareToShellExecuteFile(true));

    for (const auto& definition : { Stream::PrimaryAdminSettingsFile, Stream::BackupAdminSettingsFile })
    {
        Stream stream{ definition };
        REQUIRE_THROWS_HR(stream.Set("{}"), E_ACCESSDENIED);
        REQUIRE_THROWS_HR(stream.Remove(), E_ACCESSDENIED);
    }

    auto result = primary.Get();
    REQUIRE(result);
    REQUIRE(ReadEntireStream(*result) == value);
    REQUIRE_FALSE(Stream{ Stream::BackupAdminSettingsFile }.Get());
}

TEST_CASE("AdminFileSettings_ManagementCreation", "[settings]")
{
    if (!IsRunningAsAdminOrSystem())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    AdminFileSettingsTest test;
    const bool headless = GENERATE(false, true);
    TestCommon::UserSettingsTest settings{ "{}" };
    if (headless)
    {
        UserSettings::CreateAdminSettingsFile();
    }
    else
    {
        settings.PrepareToShellExecuteFile(true);
    }

    auto content = Stream{ Stream::PrimaryAdminSettingsFile }.Get();
    REQUIRE(content);
    REQUIRE(ReadEntireStream(*content).find("https://aka.ms/winget-settings.schema.json") != std::string::npos);
    content.reset();
    REQUIRE_FALSE(Stream{ Stream::BackupAdminSettingsFile }.Get());

    const std::string original = R"({ "visual": { "progressBar": "rainbow" } })";
    REQUIRE(Stream{ Stream::PrimaryAdminSettingsFile }.Set(original));
    UserSettings::CreateAdminSettingsFile();
    content = Stream{ Stream::PrimaryAdminSettingsFile }.Get();
    REQUIRE(content);
    REQUIRE(ReadEntireStream(*content) == original);
    REQUIRE_FALSE(Stream{ Stream::BackupAdminSettingsFile }.Get());
}

TEST_CASE("AdminFileSettings_ManagementBackup", "[settings]")
{
    if (!IsRunningAsAdminOrSystem())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    AdminFileSettingsTest test;
    const std::string primary = GENERATE(std::string{ "{}" }, std::string{ "{" }, std::string{ "[]" });
    const bool repairAfterLoad = GENERATE(false, true);
    CAPTURE(primary, repairAfterLoad);
    const std::string backup = R"({ "visual": { "progressBar": "rainbow" } })";
    REQUIRE(Stream{ Stream::PrimaryAdminSettingsFile }.Set(primary));
    REQUIRE(Stream{ Stream::BackupAdminSettingsFile }.Set(backup));

    TestCommon::UserSettingsTest settings;
    if (repairAfterLoad)
    {
        REQUIRE(Stream{ Stream::PrimaryAdminSettingsFile }.Set("{}"));
    }
    settings.PrepareToShellExecuteFile(true);

    auto content = Stream{ Stream::PrimaryAdminSettingsFile }.Get();
    REQUIRE(content);
    REQUIRE(ReadEntireStream(*content) == (repairAfterLoad ? "{}" : primary));
    content = Stream{ Stream::BackupAdminSettingsFile }.Get();
    REQUIRE(content);
    REQUIRE(ReadEntireStream(*content) == (primary == "{}" ? primary : backup));
}

TEST_CASE("AdminFileSettings_NonAdminFileAccess", "[settings]")
{
    if (!IsRunningAsAdmin() || IsRunningAsSystem())
    {
        WARN("Test requires an elevated non-SYSTEM user. Skipped.");
        return;
    }

    AdminFileSettingsTest test;
    const std::string value = "{\"value\":\"protected\"}";
    REQUIRE(Stream{ Stream::PrimaryAdminSettingsFile }.Set(value));
    REQUIRE(Stream{ Stream::BackupAdminSettingsFile }.Set(value));

    auto token = CreateNonAdminToken();
    THROW_IF_WIN32_BOOL_FALSE(ImpersonateLoggedOnUser(token.get()));
    auto revert = wil::scope_exit([]() { FAIL_FAST_IF_WIN32_BOOL_FALSE(RevertToSelf()); });
    REQUIRE_FALSE(IsRunningAsAdminOrSystem());

    for (const auto& definition : { Stream::PrimaryAdminSettingsFile, Stream::BackupAdminSettingsFile })
    {
        Stream stream{ definition };
        auto result = stream.Get();
        REQUIRE(result);
        REQUIRE(ReadEntireStream(*result) == value);
        result.reset();

        for (DWORD access : { DWORD{ GENERIC_WRITE }, DWORD{ DELETE }, DWORD{ WRITE_DAC }, DWORD{ WRITE_OWNER } })
        {
            wil::unique_hfile file{ CreateFileW(stream.GetPath().c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr) };
            DWORD error = GetLastError();
            REQUIRE_FALSE(file);
            REQUIRE(error == ERROR_ACCESS_DENIED);
        }
    }

    auto newFilePath = test.Directory.GetPath() / L"new.json";
    wil::unique_hfile file{ CreateFileW(newFilePath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr) };
    DWORD error = GetLastError();
    REQUIRE_FALSE(file);
    REQUIRE(error == ERROR_ACCESS_DENIED);
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

    std::filesystem::remove(GetPathTo(PathName::SecureSettingsForRead) / name.Name);

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

TEST_CASE("SetChangeAndReadSetting", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testsettingname" };
    std::string value1 = "This is the test setting value1";
    std::string value2 = "This is the test setting value2, which is different";
    std::string value3 = "This is the test setting value3; also different";

    name.Type = GENERATE(Type::Standard, Type::Secure);
    INFO(ToString(name.Type));

    // Set the value on stream 1
    Stream stream1{ name };
    REQUIRE(stream1.Set(value1));

    // Read the value on stream 2 to verify
    {
        Stream stream2{ name };

        auto result = stream2.Get();
        REQUIRE(static_cast<bool>(result));

        std::string settingValue = ReadEntireStream(*result);
        REQUIRE(value1 == settingValue);

        // Set the value on stream 2
        REQUIRE(stream2.Set(value2));
    }

    // Attempt to set the value on stream 1 again
    REQUIRE(!stream1.Set(value3));

    // Attempting to set again should still not work
    REQUIRE(!stream1.Set(value3));

    // Ensure that the value remains value 2
    auto result = stream1.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value2 == settingValue);

    // Now that we have read it, we can update it
    REQUIRE(stream1.Set(value3));

    result = stream1.Get();
    REQUIRE(static_cast<bool>(result));

    settingValue = ReadEntireStream(*result);
    REQUIRE(value3 == settingValue);
}

TEST_CASE("AttemptSetOnNewValue", "[settings]")
{
    StreamDefinition name{ Type::Standard, "testsettingname" };
    std::string value1 = "This is the test setting value1";
    std::string value2 = "This is the test setting value2, which is different";

    name.Type = GENERATE(Type::Standard, Type::Secure);
    INFO(ToString(name.Type));

    // Remove the stream
    Stream{ name }.Remove();

    Stream stream1{ name };
    REQUIRE(!stream1.Get());

    // Set the value on stream 2
    {
        Stream stream2{ name };
        REQUIRE(stream2.Set(value1));
    }

    // Attempt to set the value on stream 1 again
    REQUIRE(!stream1.Set(value2));

    // Attempting to set again should still not work
    REQUIRE(!stream1.Set(value2));

    // Ensure that the value remains value 2
    auto result = stream1.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value1 == settingValue);

    // Now that we have read it, we can update it
    REQUIRE(stream1.Set(value2));

    result = stream1.Get();
    REQUIRE(static_cast<bool>(result));

    settingValue = ReadEntireStream(*result);
    REQUIRE(value2 == settingValue);
}

TEST_CASE("SetAndReadSettingEncrypted", "[settings]")
{
    StreamDefinition name{ Type::Encrypted, "encrypted_test_setting" };
    std::string value = "This is the test setting value";

    Stream stream{ name };
    REQUIRE(stream.Set(value));

    auto result = stream.Get();
    REQUIRE(static_cast<bool>(result));

    std::string settingValue = ReadEntireStream(*result);
    REQUIRE(value == settingValue);

    // Ensure that the data is encrypted
    name.Type = Type::StandardFile;

    Stream streamDirect{ name };

    auto resultDirect = streamDirect.Get();
    REQUIRE(static_cast<bool>(resultDirect));

    std::string settingValueDirect = ReadEntireStream(*resultDirect);
    REQUIRE(value != settingValueDirect);
}
