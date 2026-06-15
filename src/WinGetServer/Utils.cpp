// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "Utils.h"
#pragma warning( push )
#pragma warning ( disable : 6001 6388 6553)
#include <wil/resource.h>
#pragma warning( pop )
#include <processthreadsapi.h>
#include <sddl.h>
#include <vector>

unsigned char* GetUCharString(const std::string& str)
{
    return reinterpret_cast<unsigned char*>(const_cast<char*>(str.c_str()));
}

std::string GetUserSID()
{
    HANDLE hToken = NULL;
    THROW_LAST_ERROR_IF(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken));

    DWORD dwBufferSize = 0;
    THROW_LAST_ERROR_IF(!GetTokenInformation(hToken, TokenUser, NULL, 0, &dwBufferSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER);

    std::vector<BYTE> buffer;
    buffer.resize(dwBufferSize);
    PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(&buffer[0]);

    THROW_LAST_ERROR_IF(!GetTokenInformation(hToken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize));
    THROW_HR_IF(CO_E_INVALIDSID, !IsValidSid(pTokenUser->User.Sid));

    LPSTR pszSID = NULL;
    THROW_LAST_ERROR_IF(!ConvertSidToStringSidA(pTokenUser->User.Sid, &pszSID));
    return std::string{ pszSID };
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
