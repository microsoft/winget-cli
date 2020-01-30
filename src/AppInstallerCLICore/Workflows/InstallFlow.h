// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Common.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow {

    class InstallFlow
    {
    public:
        InstallFlow(AppInstaller::Manifest::Manifest manifest, std::ostream& outStream, std::istream& inStream) :
            m_packageManifest(manifest), m_reporter(outStream, inStream) {}

        void Install();

    protected:
        AppInstaller::Manifest::Manifest m_packageManifest;
        AppInstaller::Manifest::ManifestInstaller m_selectedInstaller;
        AppInstaller::Manifest::ManifestLocalization m_selectedLocalization;
        std::filesystem::path m_downloadedInstaller;
        WorkflowReporter m_reporter;

        virtual void DownloadInstaller();
        virtual void ExecuteInstaller();

        void DownloadMsixInstaller();

        void ExecuteMsixInstaller();

        std::future<int> ExecuteMsixInstallerAsync();

        std::string GetInstallerArgs();

        std::future<DWORD> ExecuteExeInstallerAsync(const std::filesystem::path& filePath, const std::string& args);
    };
}