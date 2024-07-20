// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <windows.h>
#include <winreg.h>
#include <winerror.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

using namespace std::filesystem;

std::wstring_view RegistrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
std::wstring_view DefaultProductID = L"{A499DD5E-8DC5-4AD2-911A-BCD0263295E9}";
std::wstring_view DefaultDisplayName = L"AppInstallerTestExeInstaller";
std::wstring_view DefaultDisplayVersion = L"1.0.0.0";

void WriteModifyRepairScript(std::wofstream& script, const path& repairCompletedTextFilePath, bool isModifyScript) {
    std::wstring scriptName = isModifyScript ? L"Modify" : L"Uninstaller";
    script << L"    if /I \"%%A\"==\"/repair\" (\n"
        << L"        ECHO " << scriptName << L" Repair operation for AppInstallerTestExeInstaller.exe completed successfully > \"" << repairCompletedTextFilePath.wstring() << "\"\n"
        << L"        ECHO " << scriptName << L" Repair operation for AppInstallerTestExeInstaller.exe completed successfully\n"
        << L"        EXIT /B 0\n"
        << L"    ) else if /I \"%%A\"==\"/r\" (\n"
        << L"        ECHO " << scriptName << L" Repair operation for AppInstallerTestExeInstaller.exe completed successfully > \"" << repairCompletedTextFilePath.wstring() << "\"\n"
        << L"        ECHO " << scriptName << L" Repair operation for AppInstallerTestExeInstaller.exe completed successfully\n"
        << L"        EXIT /B 0\n"
        << L"    )";
}

void WriteModifyUninstallScript(std::wofstream& script) {
    script << L"    else if /I \"%%A\"==\"/uninstall\" (\n"
        << L"        call UninstallTestExe.bat\n"
        << L"        EXIT /B 0\n"
        << L"    ) else if /I \"%%A\"==\"/X\" (\n"
        << L"        call UninstallTestExe.bat\n"
        << L"        EXIT /B 0\n"
        << L"    )\n";
}

void WriteModifyInvalidOperationScript(std::wofstream& script) {
    script << L"echo Invalid operation\n"
        << L"EXIT /B 1\n";
}

void WriteUninstallerScript(std::wofstream& uninstallerScript, const path& uninstallerOutputTextFilePath, const std::wstring& registryKey, const path& modifyScriptPath, const path& repairCompletedTextFilePath) {
    uninstallerScript << "ECHO. >" << uninstallerOutputTextFilePath << "\n";
    uninstallerScript << "ECHO AppInstallerTestExeInstaller.exe uninstalled successfully.\n";
    uninstallerScript << "REG DELETE " << registryKey << " /f\n";
    uninstallerScript << "if exist \"" << modifyScriptPath.wstring() << "\" del \"" << modifyScriptPath.wstring() << "\"\n";
    uninstallerScript << "if exist \"" << repairCompletedTextFilePath.wstring() << "\" del \"" << repairCompletedTextFilePath.wstring() << "\"\n";
}

path GenerateUninstaller(std::wostream& out, const path& installDirectory, const std::wstring& productID, bool useHKLM)
{
    path uninstallerPath = installDirectory;
    uninstallerPath /= "UninstallTestExe.bat";

    out << "Uninstaller located at path: " << uninstallerPath << std::endl;

    path uninstallerOutputTextFilePath = installDirectory;
    uninstallerOutputTextFilePath /= "TestExeUninstalled.txt";

    path repairCompletedTextFilePath = installDirectory;
    repairCompletedTextFilePath /= "TestExeRepairCompleted.txt";

    path modifyScriptPath = installDirectory;
    modifyScriptPath /= "ModifyTestExe.bat";

    std::wstring registryKey{ useHKLM ? L"HKEY_LOCAL_MACHINE\\" : L"HKEY_CURRENT_USER\\" };
    registryKey += RegistrySubkey;
    if (!productID.empty())
    {
        registryKey += productID;
    }
    else
    {
        registryKey += DefaultProductID;
    }

    std::wofstream uninstallerScript(uninstallerPath);
    uninstallerScript << "@echo off\n";
    uninstallerScript << L"for %%A in (%*) do (\n";
    WriteModifyRepairScript(uninstallerScript, repairCompletedTextFilePath, false /*isModifyScript*/);
    uninstallerScript << ")\n";
    WriteUninstallerScript(uninstallerScript, uninstallerOutputTextFilePath, registryKey, modifyScriptPath, repairCompletedTextFilePath);

    uninstallerScript.close();

    return uninstallerPath;
}

path GenerateModifyPath(const path& installDirectory)
{
    path modifyScriptPath = installDirectory;
    modifyScriptPath /= "ModifyTestExe.bat";

    path repairCompletedTextFilePath = installDirectory;
    repairCompletedTextFilePath /= "TestExeRepairCompleted.txt";

    std::wofstream modifyScript(modifyScriptPath);

    modifyScript << L"@echo off\n";
    modifyScript << L"for %%A in (%*) do (\n";
    WriteModifyRepairScript(modifyScript, repairCompletedTextFilePath, true /*isModifyScript*/);
    WriteModifyUninstallScript(modifyScript);
    modifyScript << L")\n";
    WriteModifyInvalidOperationScript(modifyScript);

    modifyScript.close();

    return modifyScriptPath;
}

void WriteToUninstallRegistry(
    std::wostream& out,
    const std::wstring& productID,
    const path& uninstallerPath,
    const path& modifyPath,
    const std::wstring& displayName,
    const std::wstring& displayVersion,
    const std::wstring& installLocation,
    bool useHKLM,
    bool noRepair,
    bool noModify)
{
    HKEY hkey;
    LONG lReg;

    // String inputs to registry must be of wide char type
    const wchar_t* publisher = L"Microsoft Corporation";
    std::wstring uninstallString = uninstallerPath.wstring();
    std::wstring modifyPathString = modifyPath.wstring();

    DWORD version = 1;

    std::wstring registryKey{ RegistrySubkey };

    if (!productID.empty())
    {
        registryKey += productID;
        out << "Product Code overridden to: " << registryKey << std::endl;
    }
    else
    {
        registryKey += DefaultProductID;
        out << "Default Product Code used: " << registryKey << std::endl;
    }

    lReg = RegCreateKeyEx(
        useHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER,
        registryKey.c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hkey,
        NULL);

    if (lReg == ERROR_SUCCESS)
    {
        out << "Successfully opened registry key" << std::endl;

        // Set Display Name Property Value
        if (LONG res = RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName.c_str(), (DWORD)(displayName.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write DisplayName value. Error Code: " << res << std::endl;
        }

        // Set Display Version Property Value
        if (LONG res = RegSetValueEx(hkey, L"DisplayVersion", NULL, REG_SZ, (LPBYTE)displayVersion.c_str(), (DWORD)(displayVersion.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write DisplayVersion value. Error Code: " << res << std::endl;
        }

        // Set Publisher Property Value
        if (LONG res = RegSetValueEx(hkey, L"Publisher", NULL, REG_SZ, (LPBYTE)publisher, (DWORD)(wcslen(publisher) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write Publisher value. Error Code: " << res << std::endl;
        }

        // Set UninstallString Property Value
        if (LONG res = RegSetValueEx(hkey, L"UninstallString", NULL, REG_EXPAND_SZ, (LPBYTE)uninstallString.c_str(), (DWORD)(uninstallString.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write UninstallString value. Error Code: " << res << std::endl;
        }

        // Set Version Property Value
        if (LONG res = RegSetValueEx(hkey, L"Version", NULL, REG_DWORD, (LPBYTE)&version, sizeof(version)) != ERROR_SUCCESS)
        {
            out << "Failed to write Version value. Error Code: " << res << std::endl;
        }

        // Set InstallLocation Property Value
        if (LONG res = RegSetValueEx(hkey, L"InstallLocation", NULL, REG_SZ, (LPBYTE)installLocation.c_str(), (DWORD)(installLocation.length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write InstallLocation value. Error Code: " << res << std::endl;
        }

        // Set ModifyPath Property Value
        if (LONG res = RegSetValueEx(hkey, L"ModifyPath", NULL, REG_EXPAND_SZ, (LPBYTE)modifyPath.c_str(), (DWORD)(modifyPath.wstring().length() + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write ModifyPath value. Error Code: " << res << std::endl;
        }

        if(noRepair)
        {
            // Set NoRepair Property Value
            DWORD noRepairValue = 1;
            if (LONG res = RegSetValueEx(hkey, L"NoRepair", NULL, REG_DWORD, (LPBYTE)&noRepairValue, sizeof(noRepairValue)) != ERROR_SUCCESS)
            {
                out << "Failed to write NoRepair value. Error Code: " << res << std::endl;
            }
        }

        if(noModify)
        {
            // Set NoModify Property Value
            DWORD noModifyValue = 1;
            if (LONG res = RegSetValueEx(hkey, L"NoModify", NULL, REG_DWORD, (LPBYTE)&noModifyValue, sizeof(noModifyValue)) != ERROR_SUCCESS)
            {
                out << "Failed to write NoModify value. Error Code: " << res << std::endl;
            }
        }

        out << "Write to registry key completed" << std::endl;
    }
    else {
        out << "Key Creation Failed" << std::endl;
    }

    RegCloseKey(hkey);
}

void WriteToFile(const path& filePath, const std::wstringstream& content)
{
    std::wofstream file(filePath, std::ofstream::out);
    file << content.str();
    file.close();
}

void HandleRepairOperation(const std::wstring& productID, const std::wstringstream& outContent, bool useHKLM)
{
    path installDirectory;

    // Open the registry key
    HKEY hKey;
    std::wstring registryPath = std::wstring(RegistrySubkey);

    if (!productID.empty())
    {
        registryPath += productID;
    }
    else
    {
        registryPath += DefaultProductID;
    }

    LONG lReg = RegOpenKeyEx(useHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER, registryPath.c_str(), 0, KEY_READ, &hKey);

    if (lReg == ERROR_SUCCESS)
    {
        // Query the value of the InstallLocation
        wchar_t regInstallLocation[MAX_PATH];
        DWORD bufferSize = sizeof(regInstallLocation);
        lReg = RegQueryValueEx(hKey, L"InstallLocation", NULL, NULL, (LPBYTE)regInstallLocation, &bufferSize);

        if (lReg == ERROR_SUCCESS)
        {
            // Convert the InstallLocation to a path
            installDirectory = std::wstring(regInstallLocation);
        }

        // Close the registry key
        RegCloseKey(hKey);

        if(installDirectory.empty())
        {
            // We could not find the install location, so we cannot repair
            return;
        }
    }
    else
    {
        // We could not find the uninstall APR registry key, so we cannot repair
        return;
    }

    path outFilePath = installDirectory;
    outFilePath /= "TestExeRepairCompleted.txt";
    WriteToFile(outFilePath, outContent);
}

void HandleInstallationOperation(std::wostream& out, const path& installDirectory, const std::wstringstream& outContent, const std::wstring& productCode, bool useHKLM, const std::wstring& displayName, const std::wstring& displayVersion, bool noRepair, bool noModify)
{
    path outFilePath = installDirectory;
    outFilePath /= "TestExeInstalled.txt";

    std::wofstream file(outFilePath, std::ofstream::out);
    file << outContent.str();
    file.close();

    path uninstallerPath = GenerateUninstaller(out, installDirectory, productCode, useHKLM);
    path modifyPath = GenerateModifyPath(installDirectory);

    WriteToUninstallRegistry(out, productCode, uninstallerPath, modifyPath, displayName, displayVersion, installDirectory.wstring(), useHKLM, noRepair, noModify);
}

// The installer prints all args to an output file and writes to the Uninstall registry key
int wmain(int argc, const wchar_t** argv)
{
    path installDirectory = temp_directory_path();
    std::wstringstream outContent;
    std::wstring productCode;
    std::wstring displayName;
    std::wstring displayVersion;
    std::wstring aliasToExecute;
    std::wstring aliasArguments;
    bool useHKLM = false;
    bool noOperation = false;
    int exitCode = 0;
    bool isRepair = false;
    bool noRepair = false;
    bool noModify = false;

    // Output to cout by default, but swap to a file if requested
    std::wostream* out = &std::wcout;
    std::wofstream logFile;

    for (int i = 1; i < argc; i++)
    {
        outContent << argv[i] << ' ';

        // Supports custom install path.
        if (_wcsicmp(argv[i], L"/InstallDir") == 0)
        {
            if (++i < argc)
            {
                installDirectory = argv[i];
                outContent << argv[i] << ' ';
            }
        }

        // Supports custom exit code
        else if (_wcsicmp(argv[i], L"/ExitCode") == 0)
        {
            if (++i < argc)
            {
                exitCode = static_cast<int>(std::stoll(argv[i], 0, 0));
                outContent << argv[i] << ' ';
            }
        }

        // Supports custom product code ID
        else if (_wcsicmp(argv[i], L"/ProductID") == 0)
        {
            if (++i < argc)
            {
                productCode = argv[i];
                outContent << argv[i] << ' ';
            }
        }

        // Supports custom DisplayName
        else if (_wcsicmp(argv[i], L"/DisplayName") == 0)
        {
            if (++i < argc)
            {
                displayName = argv[i];
                outContent << argv[i] << ' ';
            }
        }

        // Supports custom version
        else if (_wcsicmp(argv[i], L"/Version") == 0)
        {
            if (++i < argc)
            {
                displayVersion = argv[i];
                outContent << argv[i] << ' ';
            }
        }

        // Supports log file
        else if (_wcsicmp(argv[i], L"/LogFile") == 0)
        {
            if (++i < argc)
            {
                logFile = std::wofstream(argv[i], std::wofstream::out | std::wofstream::trunc);
                out = &logFile;
                outContent << argv[i] << ' ';
            }
        }

        // Writes to HKLM
        else if (_wcsicmp(argv[i], L"/UseHKLM") == 0)
        {
            useHKLM = true;
        }

        // Executes a command alias during installation
        else if (_wcsicmp(argv[i], L"/AliasToExecute") == 0)
        {
            if (++i < argc)
            {
                aliasToExecute = argv[i];
                outContent << argv[i] << ' ';
            }
        }

        // Additional arguments to include when executing the command alias during installation
        else if (_wcsicmp(argv[i], L"/AliasArguments") == 0)
        {
            if (++i < argc)
            {
                aliasArguments = argv[i];
                outContent << argv[i] << ' ';
            }
        }

        // Supports /repair and /r to emulate repair operation using installer.
        else if (_wcsicmp(argv[i], L"/repair") == 0
            || _wcsicmp(argv[i], L"/r") == 0)
        {
            isRepair = true;
        }

        else if (_wcsicmp(argv[i], L"/NoRepair") == 0)
        {
            noRepair = true;
        }

        else if (_wcsicmp(argv[i], L"/NoModify") == 0)
        {
            noModify = true;
        }

        // Returns the success exit code to emulate being invoked by another caller.
        else if (_wcsicmp(argv[i], L"/NoOperation") == 0)
        {
            noOperation = true;
        }
    }

    if (noOperation)
    {
        return exitCode;
    }

    if (!aliasToExecute.empty())
    {
        SHELLEXECUTEINFOW execInfo = { 0 };
        execInfo.cbSize = sizeof(execInfo);
        execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        execInfo.lpFile = aliasToExecute.c_str();

        if (!aliasArguments.empty())
        {
            execInfo.lpParameters = aliasArguments.c_str();
        }
        execInfo.nShow = SW_SHOW;

        if (!ShellExecuteExW(&execInfo) || !execInfo.hProcess)
        {
            return -1;
        }
    }

    if (displayName.empty())
    {
        displayName = DefaultDisplayName;
    }

    if (displayVersion.empty())
    {
        displayVersion = DefaultDisplayVersion;
    }

    path outFilePath = installDirectory;

    if (isRepair)
    {
        outContent << L"\nInstaller Repair operation for AppInstallerTestExeInstaller.exe completed successfully.";
        HandleRepairOperation(productCode, outContent, useHKLM);
    }
    else
    {
        HandleInstallationOperation(*out, installDirectory, outContent, productCode, useHKLM, displayName, displayVersion, noRepair, noModify);
    }

    return exitCode;
}
