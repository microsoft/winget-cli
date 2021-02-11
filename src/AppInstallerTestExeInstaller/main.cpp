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

std::string_view registrySubkey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
std::string_view defaultProductID = "{A499DD5E-8DC5-4AD2-911A-BCD0263295E9}";

path GenerateUninstaller(std::ostream& out, const path& installDirectory, const std::string& productID)
{
    path uninstallerPath = installDirectory;
    uninstallerPath /= "UninstallTestExe.bat";

    out << "Uninstaller located at path: " << uninstallerPath << '\n';

    path uninstallerOutputTextFilePath = installDirectory;
    uninstallerOutputTextFilePath /= "TestExeUninstalled.txt";

    std::string registryKey{ "HKEY_CURRENT_USER\\" };
    registryKey += registrySubkey;
    if (!productID.empty())
    {
        registryKey += productID;
    }
    else
    {
        registryKey += defaultProductID;
    }

    std::ofstream uninstallerScript(uninstallerPath);
    uninstallerScript << "@echo off\n";
    uninstallerScript << "ECHO. >" << uninstallerOutputTextFilePath << "\n";
    uninstallerScript << "ECHO AppInstallerTestExeInstaller.exe uninstalled successfully.\n";
    uninstallerScript << "REG DELETE " << registryKey << " /f\n";
    uninstallerScript.close();

    return uninstallerPath;
}

void WriteToUninstallRegistry(std::ostream& out, const std::string& productID, const path& uninstallerPath)
{
    HKEY hkey;
    LONG lReg;

    // String inputs to registry must be of wide char type
    const wchar_t* displayName = L"AppInstallerTestExeInstaller";
    const wchar_t* displayVersion = L"1.0.0.0";
    const wchar_t* publisher = L"Microsoft Corporation";
    const wchar_t* uninstallString = uninstallerPath.c_str();
    DWORD version = 1;

    std::string registryKey{ registrySubkey };

    if (!productID.empty()) 
    {
        registryKey += productID;
        out << "Product Code overridden to: " << registryKey << "\n";
    }
    else 
    {
        registryKey += defaultProductID;
        out << "Default Product Code used: " << registryKey << "\n";
    }

    lReg = RegCreateKeyExA(
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
        out << "Successfully opened registry key \n";

        // Set Display Name Property Value
        if (LONG res = RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName, (DWORD)(wcslen(displayName) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write DisplayName value. Error Code: " << res << "\n";
        }

        // Set Display Version Property Value
        if (LONG res = RegSetValueEx(hkey, L"DisplayVersion", NULL, REG_SZ, (LPBYTE)displayVersion, (DWORD)(wcslen(displayVersion) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write DisplayVersion value. Error Code: " << res << "\n";
        }

        // Set Publisher Property Value
        if (LONG res = RegSetValueEx(hkey, L"Publisher", NULL, REG_SZ, (LPBYTE)publisher, (DWORD)(wcslen(publisher) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            out << "Failed to write Publisher value. Error Code: " << res << "\n";
        }

        // Set UninstallString Property Value
        if (LONG res = RegSetValueEx(hkey, L"UninstallString", NULL, REG_EXPAND_SZ, (LPBYTE)uninstallString, (DWORD)wcslen(uninstallString + 1) * sizeof(wchar_t*)) != ERROR_SUCCESS)
        {
            out << "Failed to write UninstallString value. Error Code: " << res << "\n";
        }

        // Set Version Property Value
        if (LONG res = RegSetValueEx(hkey, L"Version", NULL, REG_DWORD, (LPBYTE)&version, sizeof(version)) != ERROR_SUCCESS)
        {
            out << "Failed to write Version value. Error Code: " << res << "\n";
        }

        out << "Write to registry key completed \n";
    }
    else {
        out << "Key Creation Failed\n";
    }

    RegCloseKey(hkey);
}

// The installer prints all args to an output file and writes to the Uninstall registry key
int main(int argc, const char** argv)
{
    path installDirectory = temp_directory_path();
    std::stringstream outContent;
    std::string productCode;

    // Output to cout by default, but swap to a file if requested
    std::ostream* out = &std::cout;
    std::ofstream logFile;

    for (int i = 1; i < argc; i++)
    {
        outContent << argv[i] << ' ';

        // Supports custom install path.
        if (_stricmp(argv[i], "/InstallDir") == 0 && ++i < argc)
        {
            installDirectory = argv[i];
            outContent << argv[i] << ' ';
        }
        
        // Supports custom product code ID
        if (_stricmp(argv[i], "/ProductID") == 0 && ++i < argc)
        {
            productCode = argv[i];
        }

        // Supports log file
        if (_stricmp(argv[i], "/LogFile") == 0 && ++i < argc)
        {
            logFile = std::ofstream(argv[i], std::ofstream::out | std::ofstream::trunc);
            out = &logFile;
        }
    }

    path outFilePath = installDirectory;
    outFilePath /= "TestExeInstalled.txt";
    std::ofstream file(outFilePath, std::ofstream::out);

    file << outContent.str();

    file.close();

    path uninstallerPath = GenerateUninstaller(*out, installDirectory, productCode);

    WriteToUninstallRegistry(*out, productCode, uninstallerPath);
   
    return 0;
}
