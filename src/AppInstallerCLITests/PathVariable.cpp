#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include <Resources.h>
#include <winget/PathVariable.h>
#include <AppInstallerRuntime.h>

using namespace TestCommon;
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
