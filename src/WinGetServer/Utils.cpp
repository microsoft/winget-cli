// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Utils.h"
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <aclapi.h>
#include <namespaceapi.h>
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

// Determines whether the security descriptor of an object is at least as restrictive as the one
// that we attempted to apply when creating it.
static bool ObjectSecurityDescriptorMatches(HANDLE object, MandatoryLabelPolicy mandatoryLabelPolicy)
{
    PACL dacl = nullptr;
    PACL sacl = nullptr;
    PSECURITY_DESCRIPTOR securityDescriptorPtr = nullptr;

    // Reading the mandatory label requires only READ_CONTROL, unlike the rest of the SACL.
    THROW_IF_WIN32_ERROR(GetSecurityInfo(object, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION, nullptr, nullptr, &dacl, &sacl, &securityDescriptorPtr));
    wil::unique_hlocal_security_descriptor securityDescriptor{ securityDescriptorPtr };

    // A null DACL grants everyone full access.
    if (!dacl)
    {
        return false;
    }

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
        if (ace->AceType != ACCESS_ALLOWED_ACE_TYPE)
        {
            return false;
        }

        // We only ever grant access to the current user, so any other trustee means that this is
        // not our object, regardless of which rights it was given.
        PSID aceSid = reinterpret_cast<PSID>(&reinterpret_cast<ACCESS_ALLOWED_ACE*>(ace)->SidStart);
        if (!IsValidSid(aceSid) || !EqualSid(aceSid, tokenUser->User.Sid))
        {
            return false;
        }
    }

    if (mandatoryLabelPolicy)
    {
        if (!sacl)
        {
            return false;
        }

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
            if (GetIntegrityLevelFromLabelSid(reinterpret_cast<PSID>(&labelAce->SidStart)) < SECURITY_MANDATORY_HIGH_RID ||
                (labelAce->Mask & mandatoryLabelPolicy) != mandatoryLabelPolicy)
            {
                return false;
            }

            labelFound = true;
        }

        if (!labelFound)
        {
            return false;
        }
    }

    return true;
}

// Verifies that the security descriptor of an object is at least as restrictive as the one that
// we attempted to apply when creating it, for the objects that we have no way to carry on
// without.
static void EnsureObjectSecurityDescriptor(HANDLE object, MandatoryLabelPolicy mandatoryLabelPolicy)
{
    THROW_HR_IF(E_ACCESSDENIED, !ObjectSecurityDescriptorMatches(object, mandatoryLabelPolicy));
}

// The alias that names the server's private namespace, and the name of the boundary that
// isolates it. Namespaces are identified by both, so a namespace created with this alias outside
// of the boundary below is a different namespace entirely.
static constexpr PCWSTR s_serverNamespaceAlias = L"WinGetServer";
static constexpr PCWSTR s_serverBoundaryName = L"WinGetServerBoundary";

// The name that previously shipped clients know the server start event by. It is in the session
// object namespace, so any process running as this user can reach it.
static std::wstring GetPublicServerStartEventName()
{
    return L"WinGetServerStartEvent_" + GetUserSIDW();
}

static std::wstring GetPrivateServerStartEventName()
{
    return std::wstring{ s_serverNamespaceAlias } + L"\\WinGetServerStartEvent_" + GetUserSIDW();
}

static std::wstring GetServerMutexName()
{
    return std::wstring{ s_serverNamespaceAlias } + L"\\WinGetServerMutex_" + GetUserSIDW();
}

// The integrity level of the boundary that the server's private namespace is created in.
static DWORD GetServerBoundaryIntegrityLevel()
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    // A process can only create a namespace at or below its own integrity level, so a medium
    // integrity server would fail outright rather than have the boundary reduced to fit. The
    // security E2E tests deliberately run such a server; let it have its own namespace, which
    // is separate from the one an elevated server uses.
    if (!IsCurrentProcessAdmin())
    {
        return SECURITY_MANDATORY_MEDIUM_RID;
    }
#endif

    return SECURITY_MANDATORY_HIGH_RID;
}

// Adds a required sid to a boundary descriptor. The add functions may reallocate the descriptor,
// so the handle has to be handed to them directly and stored back afterwards.
static void AddSidToServerBoundaryDescriptor(wil::unique_boundary_descriptor& boundary, PSID sid)
{
    HANDLE raw = boundary.release();
    BOOL success = AddSIDToBoundaryDescriptor(&raw, sid);
    DWORD lastError = GetLastError();
    boundary.reset(raw);
    THROW_WIN32_IF(lastError, !success);
}

static void AddIntegrityLabelToServerBoundaryDescriptor(wil::unique_boundary_descriptor& boundary, DWORD integrityLevel)
{
    SID_IDENTIFIER_AUTHORITY mandatoryLabelAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
    wil::unique_sid integrityLabel;
    THROW_IF_WIN32_BOOL_FALSE(AllocateAndInitializeSid(&mandatoryLabelAuthority, 1, integrityLevel, 0, 0, 0, 0, 0, 0, 0, &integrityLabel));

    HANDLE raw = boundary.release();
    BOOL success = AddIntegrityLabelToBoundaryDescriptor(&raw, integrityLabel.get());
    DWORD lastError = GetLastError();
    boundary.reset(raw);
    THROW_WIN32_IF(lastError, !success);
}

// Builds the boundary that isolates the server's private namespace. Only a process running as
// this user at the given integrity level or above is within it.
static wil::unique_boundary_descriptor CreateServerBoundaryDescriptor(DWORD integrityLevel)
{
    wil::unique_boundary_descriptor boundary{ CreateBoundaryDescriptorW(s_serverBoundaryName, 0) };
    THROW_LAST_ERROR_IF(!boundary);

    auto tokenUser = GetCurrentProcessTokenUser();
    AddSidToServerBoundaryDescriptor(boundary, tokenUser->User.Sid);
    AddIntegrityLabelToServerBoundaryDescriptor(boundary, integrityLevel);

    return boundary;
}

// Creates, or opens if a server is already running, the server's private namespace.
//
// The namespace is what protects the objects in it by name rather than only by security
// descriptor. A process can only create a namespace at or below its own integrity level, so a
// lower integrity process cannot create this one ahead of the server in order to choose the
// security of the objects that the server would then open. A namespace it creates with the same
// alias at its own integrity level is a separate namespace, because a namespace is identified by
// its alias and its boundary together, so it cannot collide with this one either.
static wil::unique_private_namespace_close CreateOrOpenServerNamespace(DWORD integrityLevel)
{
    auto boundary = CreateServerBoundaryDescriptor(integrityLevel);

    // Being outside the boundary does not by itself prevent opening an existing namespace; that
    // is what the descriptor on the namespace is for.
    MandatoryLabelPolicy mandatoryLabelPolicy = GetEffectiveMandatoryLabelPolicy(
        SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP);
    auto securityDescriptor = CreateCurrentUserHighIntegritySecurityDescriptor(mandatoryLabelPolicy);

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor.get();

    wil::unique_private_namespace_close result{ CreatePrivateNamespaceW(&securityAttributes, boundary.get(), s_serverNamespaceAlias) };

    if (!result)
    {
        // Another process for this user got there first; join it so that the objects below
        // resolve to the same ones that it created.
        THROW_LAST_ERROR_IF(GetLastError() != ERROR_ALREADY_EXISTS);
        result.reset(OpenPrivateNamespaceW(boundary.get(), s_serverNamespaceAlias));
        THROW_LAST_ERROR_IF(!result);
    }

    return result;
}

// Creates or opens the mutex used to ensure a single manual activation server per user.
static wil::unique_mutex CreateOrOpenServerMutex()
{
    // The namespace prevents anything below high integrity from reaching the mutex by name at
    // all; the descriptor is the second layer, for anything that is within the boundary.
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
    result.create(name.c_str(), 0, MUTEX_ALL_ACCESS, &securityAttributes);

    // The name is inside the private namespace, so anything that could have created it ahead of
    // us is already running as this user at high integrity. The check is kept as a second layer.
    // MUTEX_ALL_ACCESS contains READ_CONTROL, so the descriptor can be read back.
    EnsureObjectSecurityDescriptor(result.get(), mandatoryLabelPolicy);

    return result;
}

// Creates, or opens if it already exists, one of the server start events.
// Returns a null handle if the event could not be created or opened, or if it already existed
// and does not enforce what we asked for.
static wil::unique_event TryCreateOrOpenServerStartEvent(const std::wstring& name, MandatoryLabelPolicy mandatoryLabelPolicy)
{
    auto securityDescriptor = CreateCurrentUserHighIntegritySecurityDescriptor(mandatoryLabelPolicy);

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor.get();

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

    if (result && !ObjectSecurityDescriptorMatches(result.get(), mandatoryLabelPolicy))
    {
        result.reset();
    }

    return result;
}

static ServerStartEvents CreateOrOpenServerStartEvents()
{
    ServerStartEvents result;

    // The public event is created only so that clients that shipped before the private one
    // existed still get signalled. Its descriptor is left exactly as those clients apply it, so
    // that neither side rejects an event legitimately created by the other.
    // No-write-up is sufficient there and is deliberately used in place of the stricter policy
    // applied to the private event: for this object type the right to signal the event is
    // reached through the generic write right, so this denies signalling while still allowing a
    // lower integrity process to wait on the event, which is harmless.
    // A squatted public event is silently ignored rather than refused, because refusing it would
    // let any process running as this user deny service simply by creating the name first.
    result.PublicEvent = TryCreateOrOpenServerStartEvent(
        GetPublicServerStartEventName(),
        GetEffectiveMandatoryLabelPolicy(SYSTEM_MANDATORY_LABEL_NO_WRITE_UP));

    MandatoryLabelPolicy privateMandatoryLabelPolicy = GetEffectiveMandatoryLabelPolicy(
        SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP);
    result.PrivateEvent = TryCreateOrOpenServerStartEvent(GetPrivateServerStartEventName(), privateMandatoryLabelPolicy);

    // There is no fallback left if the protected event is unusable.
    THROW_HR_IF(E_ACCESSDENIED, !result.PrivateEvent);

    return result;
}

void ServerStartEvents::SignalAll() const
{
    if (PublicEvent)
    {
        PublicEvent.SetEvent();
    }

    if (PrivateEvent)
    {
        PrivateEvent.SetEvent();
    }
}

bool ServerStartEvents::WaitForAny(DWORD timeoutMilliseconds) const
{
    HANDLE handles[2]{};
    DWORD count = 0;

    if (PublicEvent)
    {
        handles[count++] = PublicEvent.get();
    }

    if (PrivateEvent)
    {
        handles[count++] = PrivateEvent.get();
    }

    if (count == 0)
    {
        return false;
    }

    DWORD waitResult = WaitForMultipleObjects(count, handles, FALSE, timeoutMilliseconds);

    return (waitResult - WAIT_OBJECT_0) < count;
}

void ServerStartEvents::Reset()
{
    PublicEvent.reset();
    PrivateEvent.reset();
}

ServerSynchronization CreateOrOpenServerSynchronization(bool openMutex)
{
    ServerSynchronization result;
    result.PrivateNamespace = CreateOrOpenServerNamespace(GetServerBoundaryIntegrityLevel());
    if (openMutex)
    {
        result.Mutex = CreateOrOpenServerMutex();
    }
    result.StartEvents = CreateOrOpenServerStartEvents();
    return result;
}

#ifndef AICLI_DISABLE_TEST_HOOKS
wil::unique_private_namespace_close TryEnterHighIntegrityServerNamespace()
{
    try
    {
        return CreateOrOpenServerNamespace(SECURITY_MANDATORY_HIGH_RID);
    }
    catch (...)
    {
        return {};
    }
}
#endif
