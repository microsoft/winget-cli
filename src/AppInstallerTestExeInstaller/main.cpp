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

std::wstring GenerateUninstaller() {
    path tempPath = temp_directory_path();
    path uninstallerPath = tempPath;
    uninstallerPath /= "UninstallTestExe.bat";

    std::cout << "Uninstaller located at path: " << uninstallerPath << '\n';

    path uninstallerOutputTextFilePath = tempPath;
    uninstallerOutputTextFilePath /= "TestExeUninstalled.txt";

    std::ofstream uninstallerScript(uninstallerPath);
    uninstallerScript << "@echo off\n";
    uninstallerScript << "ECHO. >" << uninstallerOutputTextFilePath << "\n";
    uninstallerScript << "ECHO AppInstallerTestExeInstaller.exe uninstalled successfully.\n";
    uninstallerScript.close();

    return uninstallerPath.wstring();
}

void WriteToUninstallRegistry(const std::wstring& productID, const std::wstring& uninstallerPath) {
    HKEY hkey;
    LONG lReg;

    // String inputs to registry must be of wide char type
    const wchar_t* displayName = L"AppInstallerTestExeInstaller\0";
    const wchar_t* publisher = L"Microsoft Corporation\0";
    const wchar_t* uninstallString = uninstallerPath.c_str();
    DWORD version = 1;

    std::wstring registryKey = (std::wstring)registrySubkey;

    if (!productID.empty()) 
    {
        registryKey += productID;
        std::wcout << "Product Code Overrided to: " << registryKey.c_str() << "\n";
    }
    else 
    {
        registryKey += defaultProductID;
        std::wcout << "Default Product Code Used: " << registryKey.c_str() << "\n";
    }

    lReg = RegCreateKeyEx(
        HKEY_LOCAL_MACHINE,
        registryKey.c_str(),
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS | KEY_WOW64_64KEY,
        NULL,
        &hkey,
        NULL);

    if (lReg == ERROR_SUCCESS) {

        std::cout << "Successfully opened registry key \n";

        // Set Display Name Property Value
        if (LONG res = RegSetValueEx(hkey, L"DisplayName", NULL, REG_SZ, (LPBYTE)displayName, (DWORD)(wcslen(displayName) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            std::cout << "Failed to write DisplayName value. Error Code: " << res << "\n";
        }

        // Set Publisher Property Value
        if (LONG res = RegSetValueEx(hkey, L"Publisher", NULL, REG_SZ, (LPBYTE)publisher, (DWORD)(wcslen(publisher) + 1) * sizeof(wchar_t)) != ERROR_SUCCESS)
        {
            std::cout << "Failed to write Publisher value. Error Code: " << res << "\n";
        }

        // Set UninstallString Property Value
        if (LONG res = RegSetValueEx(hkey, L"UninstallString", NULL, REG_EXPAND_SZ, (LPBYTE)uninstallString, (DWORD)wcslen(uninstallString + 1) * sizeof(wchar_t*)) != ERROR_SUCCESS)
        {
            std::cout << "Failed to write UninstallString value. Error Code: " << res << "\n";
        }

        // Set Version Property Value
        if (LONG res = RegSetValueEx(hkey, L"Version", NULL, REG_DWORD, (LPBYTE)&version, sizeof(version)) != ERROR_SUCCESS)
        {
            std::cout << "Failed to write Version value. Error Code: " << res << "\n";
        }

        std::cout << "Write to registry key completed \n";
    }
    else {
        std::cout << "Key Creation Failed\n";
    }

    RegCloseKey(hkey);
}

// The installer prints all args to an output file and writes to the Uninstall registry key
int main(int argc, const char** argv)
{
    path outFilePath = temp_directory_path();
    std::wstringstream productCodeStream;
    std::stringstream outContent;
    std::wstring productCode;

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
            productCodeStream << argv[i];
        }
    }

    outFilePath /= "TestExeInstalled.txt";
    std::ofstream file(outFilePath, std::ofstream::out);

    file << outContent.str();

    file.close();

    if (!productCodeStream.str().empty()) 
    {
        productCode = productCodeStream.str();
    }
    
    std::wstring uninstallerPath = GenerateUninstaller();

    WriteToUninstallRegistry(productCode, uninstallerPath);
   
    return 0;
}


