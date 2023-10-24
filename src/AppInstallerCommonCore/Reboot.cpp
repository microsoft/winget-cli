// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"
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

    static bool* s_RegisterForRestartResult_TestHook_Override = nullptr;

    void TestHook_SetRegisterForRestartResult_Override(bool* status)
    {
        s_RegisterForRestartResult_TestHook_Override = status;
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
            AICLI_LOG(Core, Error, << "OpenProcessToken error: " << GetLastError());
            return false;
        }

        // Shutdown privilege must be enabled for this process. 
        if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &pTokenPrivileges.Privileges[0].Luid))
        {
            AICLI_LOG(Core, Error, << "LookupPrivilegeValue error: " << GetLastError());
            return false;
        }

        pTokenPrivileges.PrivilegeCount = 1;
        pTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(hToken.get(), FALSE, &pTokenPrivileges, 0, (PTOKEN_PRIVILEGES)NULL, 0))
        {
            AICLI_LOG(Core, Error, << "AdjustTokenPrivilege error: " << GetLastError());
            return false;
        }

        AICLI_LOG(Core, Info, << "Initiating reboot.");
        return ExitWindowsEx(EWX_RESTARTAPPS, SHTDN_REASON_MINOR_INSTALLATION);
    }

    bool RegisterApplicationForReboot(const std::string& commandLineArgs)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_RegisterForRestartResult_TestHook_Override)
        {
            return *s_RegisterForRestartResult_TestHook_Override;
        }
#endif

        HRESULT result = RegisterApplicationRestart(AppInstaller::Utility::ConvertToUTF16(commandLineArgs).c_str(), 0 /* Always restart process */);
        AICLI_LOG(CLI, Info, << "Register for restart with command line args: " << commandLineArgs);

        if (FAILED(result))
        {
            AICLI_LOG(Core, Error, << "RegisterApplicationRestart failed with hr: " << result);
            return false;
        }
        else
        {
            return true;
        }
    }
}
