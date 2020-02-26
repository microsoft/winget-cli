// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallerHandlerBase.h"

namespace AppInstaller::Workflow
{
    // MsixInstallerHandler handles appx/msix installers.
    class MsixInstallerHandler : public InstallerHandlerBase
    {
    public:
        MsixInstallerHandler(
            const Manifest::ManifestInstaller& manifestInstaller,
            const CLI::Invocation& args,
            WorkflowReporter& reporter) :
            InstallerHandlerBase(manifestInstaller, args, reporter) {}

        // Download method just checks installer signature hash if signature hash
        // is provided in the manifest. Otherwise, Download will download the whole
        // installer to local temp folder.
        void Download() override;

        void Install() override;

    protected:
        // If use streaming install vs download install.
        bool m_useStreaming = true;

        virtual void ExecuteInstallerAsync(const winrt::Windows::Foundation::Uri& uri);
    };
}