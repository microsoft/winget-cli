// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "InstallerHandlerBase.h"

namespace AppInstaller::Workflow
{
    class MsixInstallerHandler : public InstallerHandlerBase
    {
    public:
        MsixInstallerHandler(
            const Manifest::ManifestInstaller& manifestInstaller,
            WorkflowReporter& reporter);

        void Download() override;

        void Install() override;

    protected:
        std::future<winrt::Windows::Management::Deployment::IDeploymentResult> ExecuteInstallerAsync();
        bool m_useStreaming = true;
    };
}