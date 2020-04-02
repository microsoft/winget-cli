// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallerHandlerBase.h"
#include "ExecutionContext.h"

namespace AppInstaller::Workflow
{
    class InstallFlow : public SingleManifestWorkflow
    {
    public:
        InstallFlow(AppInstaller::CLI::Execution::Context& context) : SingleManifestWorkflow(context) {}

        // Execute will perform a query against index and do app install if a target app is found.
        // If a manifest is given with /manifest, use the manifest and no index search is performed.
        void Execute();

    protected:
        void InstallInternal();

        // Verifies the OS version is capable of supporting the application.
        bool VerifyOSVersion();

        // Creates corresponding InstallerHandler according to InstallerType
        virtual std::unique_ptr<InstallerHandlerBase> GetInstallerHandler();
    };
}