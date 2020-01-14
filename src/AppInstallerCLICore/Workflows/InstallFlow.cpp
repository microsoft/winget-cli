// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "AppInstallerRuntime.h"
#include "Manifest\ManifestInstaller.h"
#include "Manifest\Manifest.h"
#include "InstallFlow.h"
#include "AppInstallerDownloader.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow {

    void InstallFlow::Install()
    {
        DetermineLocalization();
        DetermineInstaller();

        m_reporter.ShowPackageInfo(
            m_packageManifest.Name,
            m_packageManifest.Version,
            m_packageManifest.Author,
            m_selectedLocalization.Description,
            m_selectedLocalization.Homepage,
            m_selectedLocalization.LicenseUrl
        );

        DownloadInstaller();
        ExecuteInstaller();
    }

    void InstallFlow::DetermineInstaller()
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection.");

        // Sorting the list of availlable installers according to rules defined in InstallerComparator.
        std::sort(m_packageManifest.Installers.begin(), m_packageManifest.Installers.end(), InstallerComparator());

        // If the first one is inapplicable, then no installer is applicable.
        if (!Utility::IsApplicableArchitecture(m_packageManifest.Installers[0].Arch))
        {
            m_reporter.ShowMsg("No applicable installer found.");
            throw InstallFlowException("No installer with applicable architecture found.");
        }

        // local copy to work on.
        m_selectedInstaller = m_packageManifest.Installers[0];

        // Populate default values from package manifest if individual installer field is empty.
        if (m_selectedInstaller.InstallerType.empty())
        {
            m_selectedInstaller.InstallerType = m_packageManifest.InstallerType;
        }

        // Populate default installer switches from package manifest if individual installer switch field is empty.
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

        AICLI_LOG(CLI, Info, << "Completed installer selection.");
        AICLI_LOG(CLI, Verbose, << "Selected installer arch: " << (int)m_selectedInstaller.Arch);
        AICLI_LOG(CLI, Verbose, << "Selected installer url: " << m_selectedInstaller.Url);
        AICLI_LOG(CLI, Verbose, << "Selected installer InstallerType: " << m_selectedInstaller.InstallerType);
        AICLI_LOG(CLI, Verbose, << "Selected installer scope: " << m_selectedInstaller.Scope);
        AICLI_LOG(CLI, Verbose, << "Selected installer language: " << m_selectedInstaller.Language);
    }

    void InstallFlow::DetermineLocalization()
    {
        AICLI_LOG(CLI, Info, << "Starting localization selection.");

        // local copy to work on.
        ManifestLocalization localization;

        // Pupulate default from package manifest
        localization.Description = m_packageManifest.Description;
        localization.Homepage = m_packageManifest.Homepage;
        localization.LicenseUrl = m_packageManifest.LicenseUrl;

        // Sorting the list of availlable localizations according to rules defined in LocalizationComparator.
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
        AICLI_LOG(CLI, Info, << "Completed localization selection.");
        AICLI_LOG(CLI, Verbose, << "Selected localization language: " << m_selectedLocalization.Language);
    }

    void InstallFlow::DownloadInstaller()
    {
        std::filesystem::path tempInstallerPath = Runtime::GetPathToTemp();
        tempInstallerPath /= m_packageManifest.Id + '_' + m_packageManifest.Version + '.' + m_selectedInstaller.InstallerType;

        AICLI_LOG(CLI, Info, << "Generated temp download path: " << tempInstallerPath);

        Downloader downloader;
        downloader.StartDownloadAsync(
            m_selectedInstaller.Url,
            tempInstallerPath,
            true,
            &m_reporter.GetDownloaderCallback());

        auto downloadResult = downloader.Wait();

        if (downloadResult.first == APPINSTALLER_DOWNLOAD_FAILED)
        {
            m_reporter.ShowMsg("Package download failed.");
            throw InstallFlowException("Package download failed");
        }
        else if (downloadResult.first == APPINSTALLER_DOWNLOAD_CANCELED)
        {
            m_reporter.ShowMsg("Package download canceled.");
            throw InstallFlowException("Package download canceled");
        }

        if (downloadResult.second != m_selectedInstaller.Sha256)
        {
            m_reporter.ShowMsg("Package hash verification failed.");
            throw InstallFlowException("Package hash verification failed");
        }

        AICLI_LOG(CLI, Info, << "Downloaded package hash verified");
        m_reporter.ShowMsg("Successfully verified SHA256.");

        m_downloadedInstaller = tempInstallerPath;
    }

    void InstallFlow::ExecuteInstaller()
    {
        if (m_downloadedInstaller.empty())
        {
            throw InstallFlowException("Installer not downloaded yet");
        }

        m_reporter.ShowMsg("Installing package ...");

        std::string installerArgs = GetInstallerArgs();
        AICLI_LOG(CLI, Info, << "Installer args: " << installerArgs);

        // Todo: add support for other installer types
        std::future<DWORD> installTask;
        if (Utility::ToLower(m_selectedInstaller.InstallerType) == "exe")
        {
            installTask = ExecuteExeInstallerAsync(m_downloadedInstaller, installerArgs);
        }
        else
        {
            m_reporter.ShowMsg("Installer type not supported.");
            throw InstallFlowException("Installer type not supported");
        }

        m_reporter.ShowIndefiniteSpinner(true);

        installTask.wait();

        m_reporter.ShowIndefiniteSpinner(false);

        auto installResult = installTask.get();

        if (installResult != 0)
        {
            m_reporter.ShowMsg("Install failed. Exit code: " + std::to_string(installResult));
            throw InstallFlowException("Install failed. Installer task returned: " + std::to_string(installResult));
        }

        m_reporter.ShowMsg("Successfully installed!");
    }

    std::future<DWORD> InstallFlow::ExecuteExeInstallerAsync(const std::filesystem::path& filePath, const std::string& args)
    {
        AICLI_LOG(CLI, Info, << "Staring EXE installer. Path: " << filePath);
        return std::async(std::launch::async, [&filePath, &args] {

            SHELLEXECUTEINFOA execInfo = { 0 };
            execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            execInfo.lpFile = Utility::ConvertToUTF8(filePath.c_str()).c_str();
            execInfo.lpParameters = args.c_str();
            execInfo.nShow = SW_SHOW;
            if (!ShellExecuteExA(&execInfo) || !execInfo.hProcess)
            {
                return GetLastError();
            }
            // Wait for installation to finish
            WaitForSingleObject(execInfo.hProcess, INFINITE);

            // Get exe exit code
            DWORD exitCode;
            GetExitCodeProcess(execInfo.hProcess, &exitCode);

            CloseHandle(execInfo.hProcess);

            return exitCode;
        });
    }

    std::string InstallFlow::GetInstallerArgs()
    {
        // Todo: Implement arg selection logic.
        if (m_selectedInstaller.Switches.has_value())
        {
            return m_selectedInstaller.Switches.value().Default;
        }

        return "";
    }
}