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

bool IsCurrentUserSid(PSID sid)
{
    auto [buffer, pTokenUser] = GetCurrentProcessTokenUser();
    return EqualSid(pTokenUser->User.Sid, sid) != FALSE;
}

wil::unique_event CreateOrOpenServerStartEvent()
{
    wil::unique_event result;

    for (int i = 0; !result && i < 2; ++i)
    {
        if (!result.try_create(wil::EventOptions::ManualReset, L"WinGetServerStartEvent"))
        {
            result.try_open(L"WinGetServerStartEvent");
        }
    }

    THROW_LAST_ERROR_IF(!result);

    return result;
}
