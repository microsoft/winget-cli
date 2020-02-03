// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "InstallerHandlerBase.h"

namespace AppInstaller::Workflow
{
    class ExecutableInstallerHandler : public InstallerHandlerBase
    {
    public:
        ExecutableInstallerHandler(
            const Manifest::ManifestInstaller& manifestInstaller,
            WorkflowReporter& reporter);

        void Install() override;

    protected:
        std::future<DWORD> ExecuteInstallerAsync(const std::filesystem::path& filePath, const std::string& args);
        std::string GetInstallerArgs();
        virtual void RenameDownloadedInstaller();
    };
}