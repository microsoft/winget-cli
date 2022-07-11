// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller;
using namespace AppInstaller::Runtime;
using namespace TestCommon;


bool CanWriteToPath(const std::filesystem::path& directory, const std::filesystem::path& file = "test.txt")
{
    std::ofstream out{ directory / file };
    out << "Test";
    return out.good();
}

void RequireAdminOwner(const std::filesystem::path& directory)
{
    wil::unique_hlocal_security_descriptor securityDescriptor;
    PSID ownerSID = nullptr;
    THROW_IF_WIN32_ERROR(GetNamedSecurityInfoW(directory.c_str(), SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &ownerSID, nullptr, nullptr, nullptr, &securityDescriptor));

    auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
    REQUIRE(EqualSid(adminSID.get(), ownerSID));
}

TEST_CASE("ApplyACL_CurrentUserOwner", "[runtime]")
{
    TempDirectory directory("CurrentUserOwner");
    PathDetails details;
    details.Path = directory;
    details.CurrentUser = ACEPermissions::Owner;

    details.ApplyACL();

    REQUIRE(CanWriteToPath(directory));
}

TEST_CASE("ApplyACL_RemoveWriteForUser", "[runtime]")
{
    TempDirectory directory("CurrentUserCantWrite");
    PathDetails details;
    details.Path = directory;
    details.CurrentUser = ACEPermissions::ReadExecute;

    details.ApplyACL();

    REQUIRE(!CanWriteToPath(directory));
}

TEST_CASE("ApplyACL_AdminOwner", "[runtime]")
{
    TempDirectory directory("AdminOwner");
    PathDetails details;
    details.Path = directory;
    details.Admins = ACEPermissions::Owner;

    if (IsRunningAsAdmin())
    {
        details.ApplyACL();
        RequireAdminOwner(directory);
        REQUIRE(CanWriteToPath(directory));
    }
    else
    {
        // A non-admin token cannot set the owner to be the Admins group
        REQUIRE_THROWS_HR(details.ApplyACL(), HRESULT_FROM_WIN32(ERROR_INVALID_OWNER));
    }
}
