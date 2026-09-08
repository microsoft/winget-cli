#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>
#include <Resources.h>
#include <winget/PathVariable.h>
#include <winget/Filesystem.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Registry::Environment;

TEST_CASE("PathVariable_EnforceReadOnly", "[pathVariable]")
{
    auto pathVariable = PathVariable(ScopeEnum::User, true);
    REQUIRE_THROWS_HR(pathVariable.Append("testString"), E_ACCESSDENIED);
    REQUIRE_THROWS_HR(pathVariable.Remove("testString"), E_ACCESSDENIED);
}

namespace
{
    PathVariable CreateTestPathVariable(ScopeEnum scope, const AppInstaller::Registry::Key& key, bool readOnly = false)
    {
        return PathVariable(scope, key, readOnly, /* broadcastEnvironmentChange */ false);
    }
}

TEST_CASE("PathVariable_Append_NoSemiColon", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);
    std::filesystem::path testPath{ "testString" };
    REQUIRE_FALSE(pathVariable.Contains(testPath));
    REQUIRE(pathVariable.Append(testPath));
    REQUIRE(pathVariable.Contains(testPath));

    // Verify that the path value ends with a ';' and not include ";;"
    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.back() == ';');
    REQUIRE(pathValue.find(";;") == std::string::npos);

    REQUIRE(pathVariable.Remove(testPath));
    REQUIRE_FALSE(pathVariable.Contains(testPath));
}

TEST_CASE("PathVariable_Append_WithSemicolon", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);
    std::filesystem::path testPath{ "testString;" };
    REQUIRE_FALSE(pathVariable.Contains(testPath));
    REQUIRE(pathVariable.Append(testPath));
    REQUIRE(pathVariable.Contains(testPath));

    // Verify that the path value ends with a ';' and does not include ";;"
    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.back() == ';');
    REQUIRE(pathValue.find(";;") == std::string::npos);

    REQUIRE(pathVariable.Remove(testPath));
    REQUIRE_FALSE(pathVariable.Contains(testPath));
}

std::wstring GetCurrentProcessPathVariable()
{
    size_t requiredSize;
    _wgetenv_s(&requiredSize, nullptr, 0, L"PATH");

    if (requiredSize > 0)
    {
        auto buffer = std::make_unique<wchar_t[]>(requiredSize);
        errno_t errorResult = _wgetenv_s(&requiredSize, buffer.get(), requiredSize, L"PATH");
        if (errorResult == 0)
        {
            return std::wstring(buffer.get());
        }
    }
    return {};
}

TEST_CASE("RefreshEnvironmentVariable_User", "[pathVariable]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    std::wstring testPathEntry = L"testUserPathEntry";
    auto pathVariable = AppInstaller::Registry::Environment::PathVariable(ScopeEnum::User);
    pathVariable.Append(testPathEntry);

    std::wstring initialPathValue = GetCurrentProcessPathVariable();
    bool firstCheck = initialPathValue.find(testPathEntry) != std::string::npos;

    AppInstaller::Registry::Environment::RefreshPathVariableForCurrentProcess();

    std::wstring updatedPathValue = GetCurrentProcessPathVariable();
    bool secondCheck = updatedPathValue.find(testPathEntry) != std::string::npos;

    pathVariable.Remove(testPathEntry);

    REQUIRE_FALSE(firstCheck);
    REQUIRE(secondCheck);
}

TEST_CASE("RefreshEnvironmentVariable_System", "[pathVariable]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    std::wstring testPathEntry = L"testSystemPathEntry";
    auto pathVariable = AppInstaller::Registry::Environment::PathVariable(ScopeEnum::Machine);
    pathVariable.Append(testPathEntry);

    std::wstring initialPathValue = GetCurrentProcessPathVariable();
    bool firstCheck = initialPathValue.find(testPathEntry) != std::string::npos;

    AppInstaller::Registry::Environment::RefreshPathVariableForCurrentProcess();

    std::wstring updatedPathValue = GetCurrentProcessPathVariable();
    bool secondCheck = updatedPathValue.find(testPathEntry) != std::string::npos;

    pathVariable.Remove(testPathEntry);

    REQUIRE_FALSE(firstCheck);
    REQUIRE(secondCheck);
}

TEST_CASE("VerifyPathRefreshExpandsValues", "[pathVariable]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    std::filesystem::path testEntry{ "%USERPROFILE%\\testPath" };
    auto pathVariable = AppInstaller::Registry::Environment::PathVariable(ScopeEnum::User);
    pathVariable.Append(testEntry);

    std::wstring initialPathValue = GetCurrentProcessPathVariable();
    bool firstCheck = initialPathValue.find(testEntry) != std::string::npos;

    AppInstaller::Registry::Environment::RefreshPathVariableForCurrentProcess();

    // %USERPROFILE% should be replaced with the actual path.
    std::wstring updatedPathValue = GetCurrentProcessPathVariable();
    std::wstring expandedTestPath = AppInstaller::Filesystem::GetExpandedPath(testEntry.u8string());
    bool secondCheck = updatedPathValue.find(expandedTestPath) != std::string::npos;

    pathVariable.Remove(testEntry);

    REQUIRE_FALSE(firstCheck);
    REQUIRE(secondCheck);
}


TEST_CASE("PathVariable_Append_StoresEnvironmentVariableForm", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);
    std::filesystem::path testPath = AppInstaller::Filesystem::GetExpandedPath("%LOCALAPPDATA%\\testUnexpandedPathEntry");

    REQUIRE_FALSE(pathVariable.Contains(testPath));
    REQUIRE(pathVariable.Append(testPath));

    // The entry should be stored in environment variable form rather than as a fully expanded path.
    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find("%LOCALAPPDATA%\\testUnexpandedPathEntry") != std::string::npos);

    // The entry should still be found by its expanded path, and appending it again should not duplicate it.
    REQUIRE(pathVariable.Contains(testPath));
    REQUIRE_FALSE(pathVariable.Append(testPath));

    // Removing the fully expanded path should also remove the stored environment variable form.
    REQUIRE(pathVariable.Remove(testPath));
    REQUIRE_FALSE(pathVariable.Contains(testPath));
    REQUIRE(pathVariable.GetPathValue().find("testUnexpandedPathEntry") == std::string::npos);
}

TEST_CASE("PathVariable_Append_DetectsLegacyExpandedEntry", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    std::filesystem::path testPath = AppInstaller::Filesystem::GetExpandedPath("%LOCALAPPDATA%\\testLegacyPathEntry");

    // Simulate an entry written by an older version as a fully expanded path.
    key.SetValue(L"Path", AppInstaller::Utility::ConvertToUTF16("before;" + testPath.u8string() + ";after"), REG_EXPAND_SZ);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    // Appending the same path should not duplicate the existing entry.
    REQUIRE_FALSE(pathVariable.Append(testPath));

    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find(testPath.u8string()) != std::string::npos);
    REQUIRE(pathValue.find(";;") == std::string::npos);

    // Removing the path should remove the legacy entry while preserving the other entries.
    REQUIRE(pathVariable.Remove(testPath));
    REQUIRE_FALSE(pathVariable.Contains(testPath));
    pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find(testPath.u8string()) == std::string::npos);
    REQUIRE(pathValue.find("before") != std::string::npos);
    REQUIRE(pathValue.find("after") != std::string::npos);
}

TEST_CASE("PathVariable_Remove_PreservesSubpathsAndPrefixes", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    key.SetValue(L"Path", L"before;C:\\testApp\\subDir;C:\\testApp;C:\\testAppExtended;after", REG_EXPAND_SZ);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    std::filesystem::path target = "C:\\testApp";
    std::filesystem::path subDir = "C:\\testApp\\subDir";
    std::filesystem::path prefixed = "C:\\testAppExtended";

    REQUIRE(pathVariable.Contains(target));
    REQUIRE(pathVariable.Contains(subDir));
    REQUIRE(pathVariable.Contains(prefixed));

    // Removing target must only remove C:\testApp and must NOT delete or corrupt subDir or prefixed
    REQUIRE(pathVariable.Remove(target));
    REQUIRE_FALSE(pathVariable.Contains(target));
    REQUIRE(pathVariable.Contains(subDir));
    REQUIRE(pathVariable.Contains(prefixed));

    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find("C:\\testApp\\subDir") != std::string::npos);
    REQUIRE(pathValue.find("C:\\testAppExtended") != std::string::npos);
    REQUIRE(pathValue.find("before") != std::string::npos);
    REQUIRE(pathValue.find("after") != std::string::npos);
}

TEST_CASE("PathVariable_Contains_ExactEntryMatching", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    key.SetValue(L"Path", L"C:\\ParentFolder\\Target;C:\\Tools", REG_EXPAND_SZ);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    // Should contain the exact entries
    REQUIRE(pathVariable.Contains("C:\\ParentFolder\\Target"));
    REQUIRE(pathVariable.Contains("C:\\Tools"));

    // Should NOT match suffix or prefix substrings
    REQUIRE_FALSE(pathVariable.Contains("Target"));
    REQUIRE_FALSE(pathVariable.Contains("C:\\ParentFolder"));
    REQUIRE_FALSE(pathVariable.Contains("Tool"));
}

TEST_CASE("PathVariable_ContainsAndRemove_QuotedEntries", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    key.SetValue(L"Path", L"\"C:\\Program Files\\QuotedApp\";C:\\OtherApp", REG_EXPAND_SZ);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    std::filesystem::path unquotedTarget = "C:\\Program Files\\QuotedApp";
    REQUIRE(pathVariable.Contains(unquotedTarget));
    REQUIRE(pathVariable.Remove(unquotedTarget));
    REQUIRE_FALSE(pathVariable.Contains(unquotedTarget));
    REQUIRE(pathVariable.Contains("C:\\OtherApp"));
    REQUIRE(pathVariable.GetPathValue().find("OtherApp") != std::string::npos);
}

TEST_CASE("PathVariable_Append_QuotedTarget", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);
    std::filesystem::path testPath = AppInstaller::Filesystem::GetExpandedPath("%LOCALAPPDATA%\\testQuotedAppend");

    std::wstring quotedPath = L"\"" + testPath.wstring() + L"\"";
    REQUIRE(pathVariable.Append(quotedPath));

    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find("%LOCALAPPDATA%\\testQuotedAppend") != std::string::npos);
    REQUIRE(pathVariable.Contains(testPath));
}

TEST_CASE("PathVariable_Append_EmptyOrWhitespaceTarget", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    key.SetValue(L"Path", L"C:\\Tools;", REG_EXPAND_SZ);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    REQUIRE_FALSE(pathVariable.Append(""));
    REQUIRE_FALSE(pathVariable.Append("   "));
    REQUIRE_FALSE(pathVariable.Append(";"));
    REQUIRE_FALSE(pathVariable.Append("\"\""));

    // Verify PATH was not modified
    REQUIRE(pathVariable.GetPathValue() == "C:\\Tools;");
}

TEST_CASE("PathVariable_MachineScope_DoesNotUseUserVariables", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::Machine, key);

    std::filesystem::path localAppDataPath = AppInstaller::Filesystem::GetExpandedPath("%LOCALAPPDATA%\\testMachineEntry");
    REQUIRE(pathVariable.Append(localAppDataPath));

    // For Machine scope, user variables like %LOCALAPPDATA% must NOT be used.
    std::string pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find("%LOCALAPPDATA%") == std::string::npos);
    REQUIRE(pathValue.find("%USERPROFILE%") == std::string::npos);

    // System variables like %ProgramFiles% should be used for Machine scope.
    std::filesystem::path programFilesPath = AppInstaller::Filesystem::GetExpandedPath("%ProgramFiles%\\testMachineEntry");
    REQUIRE(pathVariable.Append(programFilesPath));
    pathValue = pathVariable.GetPathValue();
    REQUIRE(pathValue.find("%ProgramFiles%\\testMachineEntry") != std::string::npos);
}

TEST_CASE("PathVariable_Contains_NFKCNormalization", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    // Decomposed 'e' + combining acute accent U+0301 vs precomposed 'é' U+00E9
    std::wstring decomposed = L"C:\\Tools\\e\u0301tude";
    std::wstring precomposed = L"C:\\Tools\\\u00E9tude";

    REQUIRE(pathVariable.Append(decomposed));

    // Both decomposed and precomposed representations should match
    REQUIRE(pathVariable.Contains(decomposed));
    REQUIRE(pathVariable.Contains(precomposed));

    // Appending with the other form should not duplicate
    REQUIRE_FALSE(pathVariable.Append(precomposed));

    // Removing with either form should succeed
    REQUIRE(pathVariable.Remove(precomposed));
    REQUIRE_FALSE(pathVariable.Contains(decomposed));
    REQUIRE_FALSE(pathVariable.Contains(precomposed));
}

TEST_CASE("PathVariable_OverlongOrMalformedEntry_DoesNotThrow", "[pathVariable]")
{
    wil::unique_hkey root = TestCommon::RegCreateVolatileTestRoot();
    AppInstaller::Registry::Key key = AppInstaller::Registry::Key::Create(root.get(), L"Environment", REG_OPTION_VOLATILE);

    // Create an entry with an unclosed % variable and an extremely long entry
    std::wstring longString(33000, L'A');
    std::wstring pathologicalPath = L"C:\\Normal;%UNCLOSED_VAR;C:\\" + longString + L";C:\\OtherApp";
    key.SetValue(L"Path", pathologicalPath, REG_EXPAND_SZ);
    auto pathVariable = CreateTestPathVariable(ScopeEnum::User, key);

    // Contains should not throw even with pathological or overlong entries
    REQUIRE(pathVariable.Contains("C:\\Normal"));
    REQUIRE(pathVariable.Contains("C:\\OtherApp"));
    REQUIRE_FALSE(pathVariable.Contains("C:\\NonExistent"));

    // Appending a normal entry should succeed without throwing
    std::filesystem::path newEntry = "C:\\NewTools";
    REQUIRE(pathVariable.Append(newEntry));
    REQUIRE(pathVariable.Contains(newEntry));
}
