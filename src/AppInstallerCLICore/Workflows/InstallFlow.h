// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "Common.h"
#include "WorkflowBase.h"
#include "InstallerHandlerBase.h"
#include "ExecutionContext.h"

namespace AppInstaller::Workflow
{
    class InstallFlow : public WorkflowBase
    {
    public:
        InstallFlow(AppInstaller::CLI::ExecutionContext& context) : WorkflowBase(context) {}

        // Execute will perform a query against index and do app install if a target app is found.
        // If a manifest is given with /manifest, use the manifest and no index search is performed.
        void Execute();

    protected:
        AppInstaller::Manifest::Manifest m_manifest;
        AppInstaller::Manifest::ManifestInstaller m_selectedInstaller;

        void GetManifest();
        void InstallInternal();

        // Creates corresponding InstallerHandler according to InstallerType
        virtual std::unique_ptr<InstallerHandlerBase> GetInstallerHandler();
    };
}