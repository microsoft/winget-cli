// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace AppInstaller::Workflow {

    class InstallFlow
    {
    public:
        InstallFlow(const AppInstaller::Manifest::Manifest manifest) : m_packageManifest(manifest) {}

        void Install();

    private:
        AppInstaller::Manifest::Manifest m_packageManifest;
        AppInstaller::Manifest::ManifestInstaller m_selectedInstaller;
        AppInstaller::Manifest::ManifestLocalization m_selectedLocalization;
        std::string m_downloadedInstaller;

        //std::filesystem::path m_downloadedInstallerPath;

        void DetermineInstaller();
        void DetermineLocalization();
        void DownloadInstaller();
        void ExecuteInstaller();
        std::string GetInstallerArgs();

        std::future<int> ExecuteExeInstallAsync(const std::string& filePath, const std::string& args);
    };
}