// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "AppInstallerRuntime.h"
#include "..\AppInstallerRepositoryCore\Manifest\ManifestInstaller.h"
#include "..\AppInstallerRepositoryCore\Manifest\Manifest.h"
#include "InstallFlow.h"
#include "AppInstallerDownloader.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow {

    void InstallFlow::Install()
    {
        DetermineInstaller();
        DownloadInstaller();
        ExecuteInstaller();
    }

    void InstallFlow::DetermineInstaller()
    {
        std::sort(m_packageManifest.Installers.begin(), m_packageManifest.Installers.end(), InstallerComparator());

        if (!Utility::IsApplicableArchitecture(m_packageManifest.Installers[0].Arch))
        {
            throw;
        }

        m_selectedInstaller = m_packageManifest.Installers[0];

        if (m_selectedInstaller.InstallerType.empty())
        {
            m_selectedInstaller.InstallerType = m_packageManifest.InstallerType;
        }

        if (m_packageManifest.Switches.has_value())
        {
            if (!m_selectedInstaller.Switches.has_value())
            {
                m_selectedInstaller.Switches.emplace(m_packageManifest.Switches.value());
            }
            else
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
        ManifestLocalization localization;

        // Pupulate default from package manifest
        localization.Description = m_packageManifest.Description;
        localization.Homepage = m_packageManifest.Homepage;
        localization.LicenseUrl = m_packageManifest.LicenseUrl;

        if (!m_packageManifest.Localization.empty())
        {
            std::sort(m_packageManifest.Localization.begin(), m_packageManifest.Localization.end(), LocalizationComparator());

            // TODO: needs to check language applicability before populating here

            if (!m_packageManifest.Localization[0].Description.empty())
            {
                localization.Description = m_packageManifest.Localization[0].Description;
            }

            if (!m_packageManifest.Localization[0].Homepage.empty())
            {
                localization.Homepage = m_packageManifest.Localization[0].Homepage;
            }

            if (!m_packageManifest.Localization[0].LicenseUrl.empty())
            {
                localization.LicenseUrl = m_packageManifest.Localization[0].LicenseUrl;
            }
        }

        m_selectedLocalization = localization;
    }

    void InstallFlow::DownloadInstaller()
    {
        std::filesystem::path tempInstallerPath = Runtime::GetPathToTemp();
        tempInstallerPath /= m_packageManifest.Id + '_' + m_packageManifest.Version + '.' + m_selectedInstaller.InstallerType;

        Downloader downloader;
        auto downloadTask = downloader.DownloadAsync(
            m_selectedInstaller.Url,
            tempInstallerPath,
            true);

        downloadTask.wait();

        auto downloadResult = downloadTask.get();

        if (downloadResult.first != APPINSTALLER_DOWNLOAD_SUCCESS)
        {
            throw;
        }

        if (downloadResult.second != m_selectedInstaller.Sha256)
        {
            throw;
        }

        m_downloadedInstaller = tempInstallerPath;
    }

    std::future<int> InstallFlow::ExecuteExeInstallerAsync(const std::filesystem::path& filePath, const std::string& args)
    {
        return std::async(std::launch::async, [&filePath, &args] {
            SHELLEXECUTEINFOA execInfo = { 0 };
            execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            execInfo.lpFile = Utility::ConvertToUTF8(filePath.c_str()).c_str();
            execInfo.lpParameters = args.c_str();
            execInfo.nShow = SW_SHOW;
            if (!ShellExecuteExA(&execInfo) || !execInfo.hProcess)
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
            installTask = ExecuteExeInstallerAsync(m_downloadedInstaller, GetInstallerArgs());
        }

        installTask.wait();

        auto installResult = installTask.get();

        if (installResult != 0)
        {
            throw;
        }

        std::cout << "done" << std::endl;
    }

    
}