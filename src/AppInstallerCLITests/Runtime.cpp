// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include <AppInstallerRuntime.h>
#include <winget/Filesystem.h>

using namespace AppInstaller;
using namespace AppInstaller::Filesystem;
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

DWORD AccessMaskFrom(ACEPermissions permissions)
{
    DWORD result = 0;

    if (permissions == ACEPermissions::All)
    {
        result |= GENERIC_ALL;
    }
    else
    {
        if (WI_IsFlagSet(permissions, ACEPermissions::Read))
        {
            result |= GENERIC_READ;
        }

        if (WI_IsFlagSet(permissions, ACEPermissions::Write))
        {
            result |= GENERIC_WRITE | FILE_DELETE_CHILD;
        }

        if (WI_IsFlagSet(permissions, ACEPermissions::Execute))
        {
            result |= GENERIC_EXECUTE;
        }
    }

    return result;
}

DWORD NormalizedAccessMaskFrom(ACEPermissions permissions)
{
    DWORD result = AccessMaskFrom(permissions);
    GENERIC_MAPPING genericMapping
    {
        FILE_GENERIC_READ,
        FILE_GENERIC_WRITE,
        FILE_GENERIC_EXECUTE,
        FILE_ALL_ACCESS,
    };

    MapGenericMask(&result, &genericMapping);
    return result;
}

void ApplyAclForTest(const std::filesystem::path& directory, const std::optional<ACEPrincipal>& owner, const std::map<ACEPrincipal, ACEPermissions>& acl, bool protectDacl = true)
{
    auto userToken = wil::get_token_information<TOKEN_USER>();
    auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
    auto systemSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID);
    PSID ownerSID = nullptr;

    struct ACEDetails
    {
        ACEPrincipal Principal;
        PSID SID;
        TRUSTEE_TYPE TrusteeType;
    };

    ACEDetails aceDetails[] =
    {
        { ACEPrincipal::CurrentUser, userToken->User.Sid, TRUSTEE_IS_USER },
        { ACEPrincipal::Admins, adminSID.get(), TRUSTEE_IS_WELL_KNOWN_GROUP },
        { ACEPrincipal::System, systemSID.get(), TRUSTEE_IS_USER },
    };

    ULONG entriesCount = 0;
    std::array<EXPLICIT_ACCESS_W, ARRAYSIZE(aceDetails)> explicitAccess;

    for (const auto& ace : aceDetails)
    {
        if (owner && owner.value() == ace.Principal)
        {
            ownerSID = ace.SID;
        }

        auto itr = acl.find(ace.Principal);
        if (itr != acl.end())
        {
            EXPLICIT_ACCESS_W& entry = explicitAccess[entriesCount++];
            entry = {};
            entry.grfAccessPermissions = AccessMaskFrom(itr->second);
            entry.grfAccessMode = SET_ACCESS;
            entry.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
            entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
            entry.Trustee.TrusteeType = ace.TrusteeType;
            entry.Trustee.ptstrName = reinterpret_cast<LPWCH>(ace.SID);
        }
    }

    wil::unique_any<PACL, decltype(&::LocalFree), ::LocalFree> appliedAcl;
    THROW_IF_WIN32_ERROR(SetEntriesInAclW(entriesCount, explicitAccess.data(), nullptr, &appliedAcl));

    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;
    if (protectDacl)
    {
        securityInformation |= PROTECTED_DACL_SECURITY_INFORMATION;
    }

    if (ownerSID)
    {
        securityInformation |= OWNER_SECURITY_INFORMATION;
    }

    std::wstring path = directory.wstring();
    THROW_IF_WIN32_ERROR(SetNamedSecurityInfoW(&path[0], SE_FILE_OBJECT, securityInformation, ownerSID, nullptr, appliedAcl.get(), nullptr));
}

void AddUnexpectedUsersAce(const std::filesystem::path& directory)
{
    wil::unique_hlocal_security_descriptor securityDescriptor;
    PACL existingDacl = nullptr;
    THROW_IF_WIN32_ERROR(GetNamedSecurityInfoW(directory.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &existingDacl, nullptr, &securityDescriptor));

    auto usersSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS);
    EXPLICIT_ACCESS_W entry = {};
    entry.grfAccessPermissions = GENERIC_READ;
    entry.grfAccessMode = GRANT_ACCESS;
    entry.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    entry.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    entry.Trustee.ptstrName = reinterpret_cast<LPWCH>(usersSID.get());

    wil::unique_any<PACL, decltype(&::LocalFree), ::LocalFree> updatedDacl;
    THROW_IF_WIN32_ERROR(SetEntriesInAclW(1, &entry, existingDacl, &updatedDacl));

    std::wstring path = directory.wstring();
    THROW_IF_WIN32_ERROR(SetNamedSecurityInfoW(&path[0], SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, updatedDacl.get(), nullptr));
}

void ApplyCombinedAceAclForTest(const std::filesystem::path& directory, const std::optional<ACEPrincipal>& owner, const std::map<ACEPrincipal, ACEPermissions>& acl)
{
    auto userToken = wil::get_token_information<TOKEN_USER>();
    auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
    auto systemSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID);
    PSID ownerSID = nullptr;

    struct ACEDetails
    {
        ACEPrincipal Principal;
        PSID SID;
    };

    ACEDetails aceDetails[] =
    {
        { ACEPrincipal::CurrentUser, userToken->User.Sid },
        { ACEPrincipal::Admins, adminSID.get() },
        { ACEPrincipal::System, systemSID.get() },
    };

    DWORD aclSize = sizeof(ACL);
    for (const auto& ace : aceDetails)
    {
        if (owner && owner.value() == ace.Principal)
        {
            ownerSID = ace.SID;
        }

        if (acl.count(ace.Principal) != 0)
        {
            aclSize += sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(ace.SID);
        }
    }

    std::vector<BYTE> aclBuffer(aclSize);
    PACL appliedAcl = reinterpret_cast<PACL>(aclBuffer.data());
    THROW_IF_WIN32_BOOL_FALSE(InitializeAcl(appliedAcl, aclSize, ACL_REVISION));

    for (const auto& ace : aceDetails)
    {
        auto itr = acl.find(ace.Principal);
        if (itr != acl.end())
        {
            THROW_IF_WIN32_BOOL_FALSE(AddAccessAllowedAceEx(
                appliedAcl,
                ACL_REVISION,
                CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
                NormalizedAccessMaskFrom(itr->second),
                ace.SID));
        }
    }

    SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION;
    if (ownerSID)
    {
        securityInformation |= OWNER_SECURITY_INFORMATION;
    }

    std::wstring path = directory.wstring();
    THROW_IF_WIN32_ERROR(SetNamedSecurityInfoW(
        &path[0],
        SE_FILE_OBJECT,
        securityInformation,
        ownerSID,
        nullptr,
        appliedAcl,
        nullptr));
}

TEST_CASE("ApplyACL_CurrentUserOwner", "[runtime]")
{
    TempDirectory directory("CurrentUserOwner");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::CurrentUser);

    details.ApplyACL();

    REQUIRE(CanWriteToPath(directory));
}

TEST_CASE("ApplyACL_RemoveWriteForUser", "[runtime]")
{
    TempDirectory directory("CurrentUserCantWrite");
    PathDetails details;
    details.Path = directory;
    details.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;

    details.ApplyACL();

    REQUIRE(!CanWriteToPath(directory));
}

TEST_CASE("ApplyACL_AdminOwner", "[runtime]")
{
    TempDirectory directory("AdminOwner");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::Admins);

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

TEST_CASE("ApplyACL_BothOwners", "[runtime]")
{
    TempDirectory directory("AdminOwner");
    PathDetails details;
    details.Path = directory;
    details.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;
    details.ACL[ACEPrincipal::System] = ACEPermissions::All;

    if (IsRunningAsSystem())
    {
        // Both cannot be owners
        REQUIRE_THROWS_HR(details.ApplyACL(), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
    }
    else
    {
        REQUIRE_NOTHROW(details.ApplyACL());
    }
}

TEST_CASE("ApplyACL_CurrentUserOwner_SystemAll", "[runtime]")
{
    TempDirectory directory("UserAndSystem");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::CurrentUser);
    details.ACL[ACEPrincipal::System] = ACEPermissions::All;

    details.ApplyACL();

    REQUIRE(CanWriteToPath(directory));
}

TEST_CASE("ShouldApplyACL_FalseWhenSecurityAlreadyMatches", "[runtime]")
{
    TempDirectory directory("ShouldApplyACLExactMatch");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::CurrentUser);
    details.ACL[ACEPrincipal::System] = ACEPermissions::All;
    details.ACL[ACEPrincipal::Admins] = ACEPermissions::All;

    details.ApplyACL();

    REQUIRE_FALSE(details.ShouldApplyACL());
}

TEST_CASE("ShouldApplyACL_FalseWhenSecurityIsSemanticallyEquivalent", "[runtime]")
{
    TempDirectory directory("ShouldApplyACLSemanticMatch");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::CurrentUser);
    details.ACL[ACEPrincipal::System] = ACEPermissions::All;
    details.ACL[ACEPrincipal::Admins] = ACEPermissions::All;

    ApplyCombinedAceAclForTest(directory, details.Owner, details.ACL);

    REQUIRE_FALSE(details.ShouldApplyACL());
}

TEST_CASE("ShouldApplyACL_TrueWhenOwnerMismatched", "[runtime]")
{
    TempDirectory directory("ShouldApplyACLMismatchedOwner");
    PathDetails actualDetails;
    actualDetails.Path = directory;
    actualDetails.SetOwner(ACEPrincipal::CurrentUser);
    actualDetails.ACL[ACEPrincipal::System] = ACEPermissions::All;
    actualDetails.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
    actualDetails.ApplyACL();

    PathDetails expectedDetails = actualDetails;
    expectedDetails.Owner = ACEPrincipal::Admins;

    REQUIRE(expectedDetails.ShouldApplyACL());
}

TEST_CASE("ShouldApplyACL_TrueWhenPermissionsMismatched", "[runtime]")
{
    TempDirectory directory("ShouldApplyACLMismatchedPermissions");
    PathDetails actualDetails;
    actualDetails.Path = directory;
    actualDetails.SetOwner(ACEPrincipal::CurrentUser);
    actualDetails.ACL[ACEPrincipal::System] = ACEPermissions::All;
    actualDetails.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
    actualDetails.ApplyACL();

    PathDetails expectedDetails = actualDetails;
    expectedDetails.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;

    REQUIRE(expectedDetails.ShouldApplyACL());
}

TEST_CASE("ShouldApplyACL_TrueWhenDaclIsNotProtected", "[runtime]")
{
    TempDirectory directory("ShouldApplyACLUnprotectedDacl");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::CurrentUser);
    details.ACL[ACEPrincipal::System] = ACEPermissions::All;
    details.ACL[ACEPrincipal::Admins] = ACEPermissions::All;

    ApplyAclForTest(directory, details.Owner, details.ACL, false);

    REQUIRE(details.ShouldApplyACL());
}

TEST_CASE("ShouldApplyACL_TrueWhenUnexpectedAceExists", "[runtime]")
{
    TempDirectory directory("ShouldApplyACLUnexpectedAce");
    PathDetails details;
    details.Path = directory;
    details.SetOwner(ACEPrincipal::CurrentUser);
    details.ACL[ACEPrincipal::System] = ACEPermissions::All;
    details.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
    details.ApplyACL();
    AddUnexpectedUsersAce(directory);

    REQUIRE(details.ShouldApplyACL());
}

TEST_CASE("SetRuntimePathStateName_AffectsTempPath", "[runtime]")
{
    constexpr std::string_view stateName = "Issue6162State";
    auto restoreStateName = wil::scope_exit([]() { Runtime::SetRuntimePathStateName("defaultState"); });

    Runtime::SetRuntimePathStateName(std::string{ stateName });

    PathDetails details = Runtime::GetPathDetailsFor(Runtime::PathName::Temp, true);

    REQUIRE(details.Path.filename() == Utility::ConvertToUTF16(stateName));
}

TEST_CASE("VerifyDevModeEnabledCheck", "[runtime]")
{
    if (!Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    bool initialState = IsDevModeEnabled();

    EnableDevMode(!initialState);
    bool modifiedState = IsDevModeEnabled();
    
    // Revert to original state.
    EnableDevMode(initialState);
    bool revertedState = IsDevModeEnabled();

    REQUIRE(modifiedState != initialState);
    REQUIRE(revertedState == initialState);
}

TEST_CASE("EnsureUserProfileNotPresentInDisplayPaths", "[runtime]")
{
    // Clear the overrides that we use when testing as they don't consider display purposes
    Runtime::TestHook_ClearPathOverrides();
    auto restorePaths = wil::scope_exit([]() { TestCommon::SetTestPathOverrides(); });

    std::filesystem::path userProfilePath = Filesystem::GetKnownFolderPath(FOLDERID_Profile);
    std::string userProfileString = userProfilePath.u8string();

    for (auto i = ToIntegral(ToEnum<Runtime::PathName>(0)); i < ToIntegral(Runtime::PathName::Max); ++i)
    {
        std::filesystem::path displayPath = GetPathTo(ToEnum<Runtime::PathName>(i), true);
        std::string displayPathString = displayPath.u8string();
        INFO(i << " = " << displayPathString);
        REQUIRE(displayPathString.find(userProfileString) == std::string::npos);
    }
}
