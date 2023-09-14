// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Reboot.h"
#include <Windows.h>

namespace AppInstaller::Reboot
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    static bool* s_HasRebootPrivilegeResult_TestHook_Override = nullptr;

    void TestHook_SetHasRebootPrivilegeResult_Override(bool* status)
    {
        s_HasRebootPrivilegeResult_TestHook_Override = status;
    }
#endif

    bool HasRebootPrivilege()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_HasRebootPrivilegeResult_TestHook_Override)
        {
            return *s_HasRebootPrivilegeResult_TestHook_Override;
        }
#endif

        wil::unique_handle hToken;
        TOKEN_PRIVILEGES pTokenPrivileges;

        // Get a token for this process. 
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            return false;
        }

        // Get the LUID for the shutdown privilege. 
        THROW_IF_WIN32_BOOL_FALSE(LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &pTokenPrivileges.Privileges[0].Luid));
        pTokenPrivileges.PrivilegeCount = 1;
        pTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        // Get the shutdown privilege for this process. 
        THROW_IF_WIN32_BOOL_FALSE(AdjustTokenPrivileges(hToken.get(), FALSE, &pTokenPrivileges, 0, (PTOKEN_PRIVILEGES)NULL, 0));

        // If the token does not have the privilege, GetLastError will return ERROR_NOT_ALL_ASSIGNED
        DWORD dwError = GetLastError();
        return dwError == ERROR_SUCCESS;
    }

    BOOL InitiateReboot()
    {
        // Reboot for uninstall check
        return ExitWindowsEx(EWX_RESTARTAPPS, SHTDN_REASON_MINOR_INSTALLATION);
    }
}
