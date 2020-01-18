// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "pch.h"
#include "Common.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow {

    class InstallFlow
    {
    public:
        InstallFlow(const AppInstaller::Manifest::Manifest manifest, std::ostream& stream) :
            m_packageManifest(manifest), m_reporter(stream) {}

        void Install();

    protected:
        AppInstaller::Manifest::Manifest m_packageManifest;
        AppInstaller::Manifest::ManifestInstaller m_selectedInstaller;
        AppInstaller::Manifest::ManifestLocalization m_selectedLocalization;
        std::filesystem::path m_downloadedInstaller;
        WorkflowReporter m_reporter;

        void DownloadInstaller();
        void ExecuteInstaller();
        std::string GetInstallerArgs();

        std::future<DWORD> ExecuteExeInstallerAsync(const std::filesystem::path& filePath, const std::string& args);
    };
}