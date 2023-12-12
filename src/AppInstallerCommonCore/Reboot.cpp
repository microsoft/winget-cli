// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"
#include "Public/winget/Reboot.h"
#include "Public/winget/Registry.h"
#include <AppInstallerRuntime.h>
#include <Windows.h>

using namespace AppInstaller::Registry;

namespace AppInstaller::Reboot
{
    namespace
    {
        constexpr std::wstring_view s_RunOnceRegistry = L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce";
    }

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

    bool RegisterRestartForWER(const std::string& commandLineArgs)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_RegisterForRestartResult_TestHook_Override)
        {
            return *s_RegisterForRestartResult_TestHook_Override;
        }
#endif

        HRESULT result = RegisterApplicationRestart(AppInstaller::Utility::ConvertToUTF16(commandLineArgs).c_str(), 0 /* Always restart process */);

        if (FAILED(result))
        {
            AICLI_LOG(Core, Error, << "RegisterApplicationRestart failed with hr: " << result);
            return false;
        }
        else
        {
            AICLI_LOG(CLI, Info, << "Register for restart with command line args: " << commandLineArgs);
            return true;
        }
    }

    bool UnregisterRestartForWER()
    {
        HRESULT result = UnregisterApplicationRestart();
        AICLI_LOG(CLI, Info, << "Application unregistered for restart.");

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

    void WriteToRunOnceRegistry(const std::string& resumeId, const std::string& commandLine)
    {
        THROW_HR_IF(E_UNEXPECTED, commandLine.size() > MAX_PATH);

        HKEY root = AppInstaller::Runtime::IsRunningAsAdmin() ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
        std::wstring subKey = std::wstring{ s_RunOnceRegistry };
        Key key = Key::OpenIfExists(root, subKey, 0, KEY_ALL_ACCESS);

        if (!key)
        {
            key = Key::Create(root, subKey);
        }

        key.SetValue(Utility::ConvertToUTF16("WingetResume-" + resumeId), Utility::ConvertToUTF16(commandLine), REG_SZ);
        AICLI_LOG(CLI, Info, << "Set RunOnce registry with value: " << commandLine);
    }
}
