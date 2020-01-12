// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow {

    class InstallFlowException : public std::runtime_error
    {
    public:
        InstallFlowException(const char* message) :
           std::runtime_error(message) {}
    };

    class InstallFlow
    {
    public:
        InstallFlow(const AppInstaller::Manifest::Manifest manifest, std::ostream& stream) :
            m_packageManifest(manifest), m_reporter(stream) {}

        void Install();

    private:
        AppInstaller::Manifest::Manifest m_packageManifest;
        AppInstaller::Manifest::ManifestInstaller m_selectedInstaller;
        AppInstaller::Manifest::ManifestLocalization m_selectedLocalization;
        std::filesystem::path m_downloadedInstaller;
        WorkflowReporter m_reporter;

        //std::filesystem::path m_downloadedInstallerPath;

        void DetermineInstaller();
        void DetermineLocalization();
        void DownloadInstaller();
        void ExecuteInstaller();
        std::string GetInstallerArgs();

        std::future<int> ExecuteExeInstallerAsync(const std::filesystem::path& filePath, const std::string& args);
    };
}