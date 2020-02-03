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
        bool m_useStreaming = true;

        virtual std::future<void> ExecuteInstallerAsync(const winrt::Windows::Foundation::Uri& uri);
    };
}