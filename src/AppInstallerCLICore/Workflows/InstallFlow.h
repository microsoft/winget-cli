// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Common.h"
#include "InstallerHandlerBase.h"
#include "WorkflowReporter.h"

namespace AppInstaller::Workflow
{
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
        WorkflowReporter m_reporter;

        virtual void ProcessManifest();

        // Creates corresponding InstallerHandler according to InstallerType
        virtual std::unique_ptr<InstallerHandlerBase> GetInstallerHandler();
    };
}