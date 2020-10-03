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

std::wstring registrySubkey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
std::wstring defaultProductID = L"{A499DD5E-8DC5-4AD2-911A-BCD0263295E9}";

std::wstring uninstallWideString;
std::wstring overriddenProductCode;

void GenerateUninstaller() {
    path tempPath = temp_directory_path();
    path uninstallerPath = tempPath;
    uninstallerPath /= "UninstallTestExe.bat";

    std::cout << "Uninstaller located at path: " << uninstallerPath << '\n';
    uninstallWideString = uninstallerPath.wstring();

    path uninstallerOutputTextFilePath = tempPath;
    uninstallerOutputTextFilePath /= "TestExeUninstalled.txt";

    std::ofstream uninstallerScript(uninstallerPath);
    uninstallerScript << "@echo off\n";
    uninstallerScript << "ECHO. >" << uninstallerOutputTextFilePath << "\n";
    uninstallerScript << "ECHO AppInstallerTestExeInstaller.exe uninstalled successfully.\n";
    uninstallerScript.close();
}

void WriteToUninstallRegistry() {
    HKEY hkey;
    LONG lReg;

    // String inputs to registry must be of wide char type
    const wchar_t* displayName = L"AppInstallerTestExeInstaller\0";
    const wchar_t* publisher = L"Microsoft Corporation\0";
    const wchar_t* uninstallString = uninstallWideString.c_str();
    DWORD version = 1;
    DWORD windowsInstaller = 1;

    std::wstring productCode;

    if (!overriddenProductCode.empty()) 
    {
        productCode = registrySubkey + overriddenProductCode;
        std::wcout << "Product Code Overrided to: " << productCode.c_str() << "\n";
    }
    else 
    {
        productCode = registrySubkey + defaultProductID;
        std::wcout << "Default Product Code Used: " << productCode.c_str() << "\n";
    }

    lReg = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        productCode.c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        NULL,
        &hkey,
        NULL);

    if (lReg == ERROR_SUCCESS) {

        std::cout << "Successfully opened registry key";
        try {
            // Set Display Name Property Value
            RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName, (DWORD)(wcslen(displayName) + 1)*sizeof(wchar_t));
            // Set Publisher Property Value
            RegSetValueEx(hkey, L"Publisher", NULL, REG_SZ, (LPBYTE)publisher, (DWORD)(wcslen(publisher) + 1) * sizeof(wchar_t));
            // Set UninstallString Property Value
            RegSetValueEx(hkey, L"UninstallString", NULL, REG_EXPAND_SZ, (LPBYTE)uninstallString, (DWORD)wcslen(uninstallString + 1)*sizeof(wchar_t*));
            // Set Version Property Value
            RegSetValueEx(hkey, L"Version", NULL, REG_DWORD, (LPBYTE)&version, sizeof(version));
            // Set WindowsInstaller Property Value
            RegSetValueEx(hkey, L"WindowsInstaller", NULL, REG_DWORD, (LPBYTE)&windowsInstaller, sizeof(windowsInstaller));
        }
        catch (int e) {
            std::cout << "An exception occurred " << e << '\n';
        };
    }
    else {
        std::cout << "Key Creation Failed";
    }

    RegCloseKey(hkey);
}

// The installer prints all args to an output file and writes to the Uninstall registry key
int main(int argc, const char** argv)
{
    path outFilePath = temp_directory_path();
    std::wstringstream productCodeStream;
    std::stringstream outContent;

    for (int i = 1; i < argc; i++)
    {
        outContent << argv[i] << ' ';

        // Supports custom install path.
        if (_stricmp(argv[i], "/InstallDir") == 0 && ++i < argc)
        {
            outFilePath = argv[i];
            outContent << argv[i] << ' ';
        }
        
        // Supports custom product code ID
        if (_stricmp(argv[i], "/ProductID") == 0 && ++i < argc)
        {
            std::cout << argc;
            productCodeStream << argv[i];
        }
    }

    outFilePath /= "TestExeInstalled.txt";
    std::ofstream file(outFilePath, std::ofstream::out);

    file << outContent.str();

    file.close();

    if (!productCodeStream.str().empty()) 
    {
        overriddenProductCode = productCodeStream.str();
    }
    
    GenerateUninstaller();

    WriteToUninstallRegistry();
   
    return 0;
}


