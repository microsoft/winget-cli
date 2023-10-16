// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "Public/winget/Reboot.h"
#include <Windows.h>

namespace AppInstaller::Reboot
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    static bool* s_InitiateRebootResult_TestHook_Override = nullptr;

    void TestHook_SetInitiateRebootResult_Override(bool* status)
    {
        s_InitiateRebootResult_TestHook_Override = status;
    }
#endif

    bool InitiateReboot()
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_InitiateRebootResult_TestHook_Override)
        {
            return *s_InitiateRebootResult_TestHook_Override;
        }
#endif

        wil::unique_handle hToken;
        TOKEN_PRIVILEGES pTokenPrivileges;

        // Get a token for this process.
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        {
            return false;
        }

        // Shutdown privilege must be enabled for this process. 
        LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &pTokenPrivileges.Privileges[0].Luid);
        pTokenPrivileges.PrivilegeCount = 1;
        pTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        AdjustTokenPrivileges(hToken.get(), FALSE, &pTokenPrivileges, 0, (PTOKEN_PRIVILEGES)NULL, 0);

        if (GetLastError != ERROR_SUCCESS)
        {
            AICLI_LOG(Core, Error, << "Failed to adjust token shutdown privilege.");
            return false;
        }

        AICLI_LOG(Core, Info, << "Initiating reboot.");
        return ExitWindowsEx(EWX_RESTARTAPPS, SHTDN_REASON_MINOR_INSTALLATION);
    }
}
