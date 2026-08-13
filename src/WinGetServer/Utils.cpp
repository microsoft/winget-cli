// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Utils.h"
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <processthreadsapi.h>
#include <sddl.h>
#include <memory>
#include <utility>

unsigned char* GetUCharString(const std::string& str)
{
    return reinterpret_cast<unsigned char*>(const_cast<char*>(str.c_str()));
}

static std::pair<std::unique_ptr<BYTE[]>, PTOKEN_USER> GetCurrentProcessTokenUser()
{
    wil::unique_handle tokenHandle;
    THROW_LAST_ERROR_IF(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, tokenHandle.put()));

    DWORD dwBufferSize = 0;
    THROW_LAST_ERROR_IF(!GetTokenInformation(tokenHandle.get(), TokenUser, NULL, 0, &dwBufferSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER);

    auto buffer = std::make_unique<BYTE[]>(dwBufferSize);
    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.get());

    THROW_LAST_ERROR_IF(!GetTokenInformation(tokenHandle.get(), TokenUser, pTokenUser, dwBufferSize, &dwBufferSize));
    THROW_HR_IF(CO_E_INVALIDSID, !IsValidSid(pTokenUser->User.Sid));

    return { std::move(buffer), pTokenUser };
}

std::string GetUserSID()
{
    auto [buffer, pTokenUser] = GetCurrentProcessTokenUser();
    LPSTR pszSID = NULL;
    THROW_LAST_ERROR_IF(!ConvertSidToStringSidA(pTokenUser->User.Sid, &pszSID));
    return std::string{ pszSID };
}

std::pair<std::unique_ptr<BYTE[]>, PSID> GetUserSidBinary()
{
    auto [buffer, pTokenUser] = GetCurrentProcessTokenUser();
    PSID sid = pTokenUser->User.Sid;
    return { std::move(buffer), sid };
}

static std::wstring GetUserSIDW()
{
    auto [buffer, pTokenUser] = GetCurrentProcessTokenUser();
    LPWSTR pszSID = NULL;
    THROW_LAST_ERROR_IF(!ConvertSidToStringSidW(pTokenUser->User.Sid, &pszSID));
    wil::unique_hlocal_string sidPtr{ pszSID };
    return std::wstring{ pszSID };
}

std::string GetServerEndpointName()
{
    return "WinGetServerManualActivation_" + GetUserSID();
}

// Builds a security descriptor granting the current user full access and requiring
// high integrity, matching the protection applied to the RPC interface.
static wil::unique_hlocal_security_descriptor CreateCurrentUserHighIntegritySecurityDescriptor()
{
    std::wstring securityDescriptorString = L"D:(A;;GA;;;" + GetUserSIDW() + L")S:(ML;;NW;;;HI)";
    wil::unique_hlocal_security_descriptor result;
    THROW_LAST_ERROR_IF(!ConvertStringSecurityDescriptorToSecurityDescriptorW(securityDescriptorString.c_str(), SDDL_REVISION_1, &result, nullptr));
    return result;
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
    auto securityDescriptor = CreateCurrentUserHighIntegritySecurityDescriptor();

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor.get();

    std::wstring name = GetServerMutexName();

    wil::unique_mutex result;
    THROW_LAST_ERROR_IF(!result.try_create(name.c_str(), 0, MUTEX_ALL_ACCESS, &securityAttributes));

    return result;
}

wil::unique_event CreateOrOpenServerStartEvent()
{
    // The DACL keeps the event private to this user and the mandatory label prevents a lower
    // integrity process running as this user from signalling it early to defeat the wait below.
    auto securityDescriptor = CreateCurrentUserHighIntegritySecurityDescriptor();

    SECURITY_ATTRIBUTES securityAttributes{};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.lpSecurityDescriptor = securityDescriptor.get();

    std::wstring name = GetServerStartEventName();

    wil::unique_event result;

    for (int i = 0; !result && i < 2; ++i)
    {
        if (!result.try_create(wil::EventOptions::ManualReset, name.c_str(), &securityAttributes))
        {
            result.try_open(name.c_str());
        }
    }

    THROW_LAST_ERROR_IF(!result);

    return result;
}
