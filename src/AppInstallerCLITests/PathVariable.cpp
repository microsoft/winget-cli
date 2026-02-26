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

TEST_CASE("PathVariable_Append_NoSemiColon", "[pathVariable]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    auto pathVariable = PathVariable(ScopeEnum::User);
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
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    auto pathVariable = PathVariable(ScopeEnum::User);
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
