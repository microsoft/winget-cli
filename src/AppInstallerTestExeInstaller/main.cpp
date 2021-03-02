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

std::wstring_view registrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
std::wstring_view defaultProductID = L"{A499DD5E-8DC5-4AD2-911A-BCD0263295E9}";
std::wstring_view defaultVersion = L"1.0.0.0";

path GenerateUninstaller(std::wostream& out, const path& installDirectory, const std::wstring& productID)
{
    path uninstallerPath = installDirectory;
    uninstallerPath /= "UninstallTestExe.bat";

    out << "Uninstaller located at path: " << uninstallerPath << std::endl;

    path uninstallerOutputTextFilePath = installDirectory;
    uninstallerOutputTextFilePath /= "TestExeUninstalled.txt";

    std::wstring registryKey{ L"HKEY_CURRENT_USER\\" };
    registryKey += registrySubkey;
    if (!productID.empty())
    {
        registryKey += productID;
    }
    else
    {
        registryKey += defaultProductID;
    }

    std::wofstream uninstallerScript(uninstallerPath);
    uninstallerScript << "@echo off\n";
    uninstallerScript << "ECHO. >" << uninstallerOutputTextFilePath << "\n";
    uninstallerScript << "ECHO AppInstallerTestExeInstaller.exe uninstalled successfully.\n";
    uninstallerScript << "REG DELETE " << registryKey << " /f\n";
    uninstallerScript.close();

    return uninstallerPath;
}

void WriteToUninstallRegistry(std::wostream& out, const std::wstring& productID, const path& uninstallerPath, const std::wstring& displayVersion)
{
    HKEY hkey;
    LONG lReg;

    // String inputs to registry must be of wide char type
    const wchar_t* displayName = L"AppInstallerTestExeInstaller";
    const wchar_t* publisher = L"Microsoft Corporation";
    std::wstring uninstallString = uninstallerPath.wstring();
    DWORD version = 1;

    std::wstring registryKey{ registrySubkey };

    if (!productID.empty()) 
    {
        registryKey += productID;
        out << "Product Code overridden to: " << registryKey << std::endl;
    }
    else 
    {
        registryKey += defaultProductID;
        out << "Default Product Code used: " << registryKey << std::endl;
    }

    lReg = RegCreateKeyEx(
        HKEY_CURRENT_USER,
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
        if (LONG res = RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName, (DWORD)(wcslen(displayName) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
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

        out << "Write to registry key completed" << std::endl;
    }
    else {
        out << "Key Creation Failed" << std::endl;
    }

    RegCloseKey(hkey);
}

// The installer prints all args to an output file and writes to the Uninstall registry key
int wmain(int argc, const wchar_t** argv)
{
    path installDirectory = temp_directory_path();
    std::wstringstream outContent;
    std::wstring productCode;
    std::wstring version;

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
        
        // Supports custom product code ID
        else if (_wcsicmp(argv[i], L"/ProductID") == 0)
        {
            if (++i < argc)
            {
                productCode = argv[i];
            }
        }

        // Supports custom version
        else if (_wcsicmp(argv[i], L"/Version") == 0)
        {
            if (++i < argc)
            {
                version = argv[i];
            }
        }

        // Supports log file
        else if (_wcsicmp(argv[i], L"/LogFile") == 0)
        {
            if (++i < argc)
            {
                logFile = std::wofstream(argv[i], std::wofstream::out | std::wofstream::trunc);
                out = &logFile;
            }
        }
    }

    if (version.empty())
    {
        version = defaultVersion;
    }

    path outFilePath = installDirectory;
    outFilePath /= "TestExeInstalled.txt";
    std::wofstream file(outFilePath, std::ofstream::out);

    file << outContent.str();

    file.close();

    path uninstallerPath = GenerateUninstaller(*out, installDirectory, productCode);

    WriteToUninstallRegistry(*out, productCode, uninstallerPath, version);
   
    return 0;
}
