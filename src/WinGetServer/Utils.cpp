// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Utils.h"
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <aclapi.h>
#include <processthreadsapi.h>
#include <sddl.h>
#include <string_view>

unsigned char* GetUCharString(const std::string& str)
{
    return reinterpret_cast<unsigned char*>(const_cast<char*>(str.c_str()));
}

static wil::unique_tokeninfo_ptr<TOKEN_USER> GetCurrentProcessTokenUser()
{
    // The process token is used rather than the thread's effective token so that the identity
    // does not change if a thread happens to be impersonating.
    auto tokenUser = wil::get_token_information<TOKEN_USER>(GetCurrentProcessToken());
    THROW_HR_IF(CO_E_INVALIDSID, !IsValidSid(tokenUser->User.Sid));
    return tokenUser;
}

std::string GetUserSID()
{
    auto tokenUser = GetCurrentProcessTokenUser();
    LPSTR pszSID = NULL;
    THROW_LAST_ERROR_IF(!ConvertSidToStringSidA(tokenUser->User.Sid, &pszSID));
    wil::unique_hlocal_ansistring sidPtr{ pszSID };
    return std::string{ pszSID };
}

wil::unique_tokeninfo_ptr<TOKEN_USER> GetBinaryUserSID()
{
    return GetCurrentProcessTokenUser();
}

static std::wstring GetUserSIDW()
{
    auto tokenUser = GetCurrentProcessTokenUser();
    LPWSTR pszSID = NULL;
    THROW_LAST_ERROR_IF(!ConvertSidToStringSidW(tokenUser->User.Sid, &pszSID));
    wil::unique_hlocal_string sidPtr{ pszSID };
    return std::wstring{ pszSID };
}

std::string GetServerEndpointName()
{
    return "WinGetServerManualActivation_" + GetUserSID();
}

#ifndef AICLI_DISABLE_TEST_HOOKS
bool IsCurrentProcessAdmin()
{
    BOOL result = FALSE;
    wil::unique_sid adminGroup;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    THROW_IF_WIN32_BOOL_FALSE(AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup));
    THROW_IF_WIN32_BOOL_FALSE(CheckTokenMembership(nullptr, adminGroup.get(), &result));

    return result != FALSE;
}
#endif

// The mandatory label policy that is applied to, and required of, a given object.
using MandatoryLabelPolicy = DWORD;

// Converts a mandatory label policy into the rights portion of an SDDL ace string.
// See https://learn.microsoft.com/en-us/windows/win32/secauthz/ace-strings for decoder ring.
static std::wstring GetMandatoryLabelPolicyString(MandatoryLabelPolicy policy)
{
    std::wstring result;

    if (policy & SYSTEM_MANDATORY_LABEL_NO_READ_UP)
    {
        result += L"NR";
    }

    if (policy & SYSTEM_MANDATORY_LABEL_NO_WRITE_UP)
    {
        result += L"NW";
    }

    if (policy & SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP)
    {
        result += L"NX";
    }

    return result;
}

// Determines the mandatory label policy that will actually be applied to a newly created object.
static MandatoryLabelPolicy GetEffectiveMandatoryLabelPolicy(MandatoryLabelPolicy policy)
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    // A process below high integrity is not allowed to apply a high integrity label, and the
    // creation call fails outright rather than the label being reduced to fit. The security
    // E2E tests deliberately run a medium integrity server process in order to check that the
    // client refuses it, so omit the label in that case and keep the rest of the descriptor.
    if (!IsCurrentProcessAdmin())
    {
        return 0;
    }
#endif

    return policy;
}

// Builds a security descriptor granting the current user full access and requiring high
// integrity.
static wil::unique_hlocal_security_descriptor CreateCurrentUserHighIntegritySecurityDescriptor(MandatoryLabelPolicy mandatoryLabelPolicy)
{
    std::wstring securityDescriptorString = L"D:(A;;GA;;;" + GetUserSIDW() + L")";

    if (mandatoryLabelPolicy)
    {
        securityDescriptorString += L"S:(ML;;" + GetMandatoryLabelPolicyString(mandatoryLabelPolicy) + L";;;HI)";
    }

    wil::unique_hlocal_security_descriptor result;
    THROW_LAST_ERROR_IF(!ConvertStringSecurityDescriptorToSecurityDescriptorW(securityDescriptorString.c_str(), SDDL_REVISION_1, &result, nullptr));
    return result;
}

// Retrieves the integrity level of a mandatory label ace's sid, which is carried by its last
// subauthority.
static DWORD GetIntegrityLevelFromLabelSid(PSID sid)
{
    THROW_HR_IF(CO_E_INVALIDSID, !IsValidSid(sid));
    PUCHAR subAuthorityCount = GetSidSubAuthorityCount(sid);
    THROW_HR_IF(CO_E_INVALIDSID, !subAuthorityCount || *subAuthorityCount == 0);
    return *GetSidSubAuthority(sid, static_cast<DWORD>(*subAuthorityCount - 1));
}

// Verifies that the security descriptor of an object is at least as restrictive as the one that
// we attempted to apply when creating it.
static void EnsureObjectSecurityDescriptor(HANDLE object, MandatoryLabelPolicy mandatoryLabelPolicy)
{
    PACL dacl = nullptr;
    PACL sacl = nullptr;
    PSECURITY_DESCRIPTOR securityDescriptorPtr = nullptr;

    // Reading the mandatory label requires only READ_CONTROL, unlike the rest of the SACL.
    THROW_IF_WIN32_ERROR(GetSecurityInfo(object, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, &sacl, &securityDescriptorPtr));
    wil::unique_hlocal_security_descriptor securityDescriptor{ securityDescriptorPtr };

    // A null DACL grants everyone full access.
    THROW_HR_IF(E_ACCESSDENIED, !dacl);

    auto tokenUser = GetCurrentProcessTokenUser();

    ACL_SIZE_INFORMATION aclSizeInformation{};
    THROW_IF_WIN32_BOOL_FALSE(GetAclInformation(dacl, &aclSizeInformation, sizeof(aclSizeInformation), AclSizeInformation));

    for (DWORD i = 0; i < aclSizeInformation.AceCount; ++i)
    {
        ACE_HEADER* ace = nullptr;
        THROW_IF_WIN32_BOOL_FALSE(GetAce(dacl, i, reinterpret_cast<LPVOID*>(&ace)));

        // Denied access can only be more restrictive than what we asked for.
        if (ace->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            continue;
        }

        // Anything that is not a plain allow ace grants access in a form that we never apply and
        // cannot evaluate here; treat it as not matching.
        THROW_HR_IF(E_ACCESSDENIED, ace->AceType != ACCESS_ALLOWED_ACE_TYPE);

        // We only ever grant access to the current user, so any other trustee means that this is
        // not our object, regardless of which rights it was given.
        PSID aceSid = reinterpret_cast<PSID>(&reinterpret_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart);
        THROW_HR_IF(E_ACCESSDENIED, !IsValidSid(aceSid) || !EqualSid(aceSid, tokenUser->User.Sid));
    }

    if (mandatoryLabelPolicy)
    {
        THROW_HR_IF(E_ACCESSDENIED, !sacl);

        ACL_SIZE_INFORMATION saclSizeInformation{};
        THROW_IF_WIN32_BOOL_FALSE(GetAclInformation(sacl, &saclSizeInformation, sizeof(saclSizeInformation), AclSizeInformation));

        bool labelFound = false;

        for (DWORD i = 0; i < saclSizeInformation.AceCount; ++i)
        {
            ACE_HEADER* ace = nullptr;
            THROW_IF_WIN32_BOOL_FALSE(GetAce(sacl, i, reinterpret_cast<LPVOID*>(&ace)));

            if (ace->AceType != SYSTEM_MANDATORY_LABEL_ACE_TYPE)
            {
                continue;
            }

            SYSTEM_MANDATORY_LABEL_ACE* labelAce = reinterpret_cast<SYSTEM_MANDATORY_LABEL_ACE*>(ace);

            // An object labelled above high integrity is also acceptable; it denies at least as
            // much as the label that we apply.
            THROW_HR_IF(E_ACCESSDENIED, GetIntegrityLevelFromLabelSid(reinterpret_cast<PSID>(&labelAce->SidStart)) < SECURITY_MANDATORY_HIGH_RID);
            THROW_HR_IF(E_ACCESSDENIED, (labelAce->Mask & mandatoryLabelPolicy) != mandatoryLabelPolicy);

            labelFound = true;
        }

        THROW_HR_IF(E_ACCESSDENIED, !labelFound);
    }
}

static std::wstring GetServerStartEventName()
{
    return L"WinGetServerStartEvent_" + GetUserSIDW();
}

static std::wstring GetServerMutexName()
{
    return L"WinGetServerMutex_" + GetUserSIDW();
}

wil::unique_mutex CreateOrOpenServerMutex()
{
    // The mandatory label prevents a lower integrity process running as this user from
    // acquiring the mutex to keep the server from starting.
    // No-write-up on its own is not enough here: for this object type the right to wait on the
    // mutex is reached through the generic execute right and the right to query it through the
    // generic read right, so a lower integrity process would still be able to take ownership of
    // it and hold it indefinitely. All three of no-read-up, no-write-up and no-execute-up are
    // required to deny that. Nothing below high integrity has any legitimate use for the mutex.
    MandatoryLabelPolicy mandatoryLabelPolicy = GetEffectiveMandatoryLabelPolicy(
        SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP);
    auto securityDescriptor = CreateCurrentUserHighIntegritySecurityDescriptor(mandatoryLabelPolicy);

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor.get();

    std::wstring name = GetServerMutexName();

    wil::unique_mutex result;
    THROW_LAST_ERROR_IF(!result.try_create(name.c_str(), 0, MUTEX_ALL_ACCESS, &securityAttributes));

    // MUTEX_ALL_ACCESS contains READ_CONTROL, so the descriptor can be read back.
    EnsureObjectSecurityDescriptor(result.get(), mandatoryLabelPolicy);

    return result;
}

wil::unique_event CreateOrOpenServerStartEvent()
{
    // The DACL keeps the event private to this user and the mandatory label prevents a lower
    // integrity process running as this user from signalling it early to defeat the wait below.
    // No-write-up is sufficient and is deliberately used in place of the stricter policy applied
    // to the mutex: for this object type the right to signal the event is reached through the
    // generic write right, so this denies signalling while still allowing a lower integrity
    // process to wait on the event, which is harmless.
    MandatoryLabelPolicy mandatoryLabelPolicy = GetEffectiveMandatoryLabelPolicy(SYSTEM_MANDATORY_LABEL_NO_WRITE_UP);
    auto securityDescriptor = CreateCurrentUserHighIntegritySecurityDescriptor(mandatoryLabelPolicy);

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor.get();

    std::wstring name = GetServerStartEventName();

    wil::unique_event result;

    for (int i = 0; !result && i < 2; ++i)
    {
        if (!result.try_create(wil::EventOptions::ManualReset, name.c_str(), &securityAttributes))
        {
            // READ_CONTROL is requested in addition to the access that the callers need so that
            // the descriptor can be read back below.
            result.try_open(name.c_str(), SYNCHRONIZE | EVENT_MODIFY_STATE | READ_CONTROL);
        }
    }

    THROW_LAST_ERROR_IF(!result);

    EnsureObjectSecurityDescriptor(result.get(), mandatoryLabelPolicy);

    return result;
}
