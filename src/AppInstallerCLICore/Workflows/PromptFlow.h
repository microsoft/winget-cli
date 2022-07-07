// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    // Shows all the prompts required for a single package, e.g. for package agreements
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: None
    struct ShowPromptsForSinglePackage : public WorkflowTask
    {
        ShowPromptsForSinglePackage(bool ensureAgreementsAcceptance) :
            WorkflowTask("ShowPromptsForSinglePackage"), m_ensureAgreementsAcceptance(ensureAgreementsAcceptance) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_ensureAgreementsAcceptance;
    };

    // Shows all the prompts required for multiple package, e.g. for package agreements
    // Required Args: None
    // Inputs: PackagesToInstall
    // Outputs: None
    struct ShowPromptsForMultiplePackages : public WorkflowTask
    {
        ShowPromptsForMultiplePackages(bool ensureAgreementsAcceptance) :
            WorkflowTask("ShowPromptsForMultiplePackages"), m_ensureAgreementsAcceptance(ensureAgreementsAcceptance) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_ensureAgreementsAcceptance;
    };
}