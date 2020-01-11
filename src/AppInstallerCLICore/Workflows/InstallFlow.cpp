// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "AppInstallerRuntime.h"
#include "..\AppInstallerRepositoryCore\Manifest\ManifestInstaller.h"
#include "..\AppInstallerRepositoryCore\Manifest\Manifest.h"
#include "InstallFlow.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow {

    void InstallFlow::Install()
    {
        DetermineInstaller();
        DownloadInstaller();
        ExecuteInstaller();
    }

    struct InstallerComparator
    {
        bool operator() (const ManifestInstaller& struct1, const ManifestInstaller& struct2)
        {
            // Todo: Comapre only architecture for now
            if (Runtime::IsApplicableArchitecture(struct1.Arch))
            {
                return false;
            }

            return true;
        }
    };

    void InstallFlow::DetermineInstaller()
    {
        std::sort(m_packageManifest.Installers.begin(), m_packageManifest.Installers.end(), InstallerComparator());

        if (!Runtime::IsApplicableArchitecture(m_packageManifest.Installers[0].Arch))
        {
            throw;
        }

        m_selectedInstaller = m_packageManifest.Installers[0];

        if (m_selectedInstaller.InstallerType.empty())
        {
            m_selectedInstaller.InstallerType = m_packageManifest.InstallerType;
        }

        if (!m_selectedInstaller.Switches.has_value())
        {
            if (m_packageManifest.Switches.has_value())
            {
                m_selectedInstaller.Switches.emplace(m_packageManifest.Switches.value());
            }
        }
        else
        {
            if (m_packageManifest.Switches.has_value())
            {
                if (m_selectedInstaller.Switches.value().Default.empty())
                {
                    m_selectedInstaller.Switches.value().Default = m_packageManifest.Switches.value().Default;
                }
                if (m_selectedInstaller.Switches.value().Silent.empty())
                {
                    m_selectedInstaller.Switches.value().Silent = m_packageManifest.Switches.value().Silent;
                }
                if (m_selectedInstaller.Switches.value().Verbose.empty())
                {
                    m_selectedInstaller.Switches.value().Verbose = m_packageManifest.Switches.value().Verbose;
                }
            }
        }
    }

    void InstallFlow::DetermineLocalization()
    {
        //std::string preferredArchitecture = Runtime::GetPreferredArchitecture();
        //for ()
    }

    void InstallFlow::DownloadInstaller()
    {
        std::string tempInstallerPath = Utility::ConvertToUTF8(Runtime::GetPathToTemp().c_str()) + m_packageManifest.Id + m_packageManifest.Version + '.' + m_selectedInstaller.InstallerType;
        
        std::cout << "Download started" << std::endl;

        HINTERNET hSession = InternetOpenA(
            "appinstaller-cli",
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            0);

        if (!hSession)
        {
            // TODO : failed
            std::cout << "InternetOpenA failed" << std::endl;
        }

        std::cout << "InternetOpenA done" << std::endl;

        HINTERNET hUrlFile = InternetOpenUrlA(
            hSession,
            m_selectedInstaller.Url.c_str(),
            NULL,
            0,
            INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS,
            0);

        if (!hUrlFile)
        {
            // TODO: failed
            std::cout << "InternetOpenUrlA failed:" << GetLastError() << std::endl;

        }

        // check http return status
        DWORD requestStatusCode;
        DWORD cbStatusCode = sizeof(requestStatusCode);

        std::cout << "InternetOpenUrlA done" << std::endl;

        if (HttpQueryInfoA(hUrlFile,
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &requestStatusCode,
            &cbStatusCode,
            nullptr))
        {

        }

        LONGLONG dwContentLength;
        DWORD cbContentLength = sizeof(dwContentLength);

        HttpQueryInfo(
            hUrlFile,
            HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER64,
            &dwContentLength,
            &cbContentLength,
            nullptr);

        std::cout << "content length: " << dwContentLength << std::endl;

        const int kBufferSize = 1024 * 1024;
        char* buffer = new char[kBufferSize];

        BOOL keepReading = true;
        DWORD bytesRead = 0;

        std::ofstream outfile(tempInstallerPath, std::ofstream::binary);

        std::cout << "start read" << std::endl;

        do
        {

            keepReading = InternetReadFile(hUrlFile, buffer, kBufferSize, &bytesRead);
            outfile.write(buffer, kBufferSize);
            std::cout << "reading...  " << bytesRead << std::endl;
        } while (keepReading && bytesRead != 0);


        std::cout << "done read" << std::endl;

        m_downloadedInstaller = tempInstallerPath;
        
    }

    std::future<int> InstallFlow::ExecuteExeInstallAsync(const std::string& filePath, const std::string& args)
    {
        return std::async(std::launch::async, [&filePath, &args] {
            SHELLEXECUTEINFOA execInfo = { 0 };
            execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            execInfo.lpFile = filePath.c_str();
            execInfo.lpParameters = args.c_str();
            execInfo.nShow = SW_SHOW;
            if (!ShellExecuteExA(&execInfo))
            {
                return 1;
            }
            // Wait for installation to finish
            WaitForSingleObject(execInfo.hProcess, INFINITE);
            CloseHandle(execInfo.hProcess);
            return 0;
            });
    }

    std::string InstallFlow::GetInstallerArgs()
    {
        if (m_selectedInstaller.Switches.has_value())
        {
            return m_selectedInstaller.Switches.value().Default;
        }

        return "";
    }

    void InstallFlow::ExecuteInstaller()
    {
        if (m_downloadedInstaller.empty())
        {
            throw;
        }

        std::future<int> installTask;
        if (m_selectedInstaller.InstallerType.compare("exe") == 0)
        {
           
            installTask = ExecuteExeInstallAsync(m_downloadedInstaller, GetInstallerArgs());
        }

        while (installTask.wait_for(std::chrono::milliseconds(1000)) != std::future_status::ready)
        {
            std::cout << "installing..." << std::endl;
        }

        std::cout << "done" << std::endl;
    }

    
}