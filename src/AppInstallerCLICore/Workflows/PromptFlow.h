// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include "TableOutput.h"

namespace AppInstaller::CLI::Workflow
{
    // Displays numbered choices and reads a selection when prompting is available.
    // Required Args: None
    // Inputs: None
    // Outputs: PromptSelection (zero-based index, or nullopt if prompting is unavailable)
    struct PromptForSelection : public WorkflowTask
    {
        PromptForSelection(Execution::TableOutputBase& table, size_t count, Resource::LocString title,
            Resource::LocString invalid) :
            WorkflowTask("PromptForSelection"), m_table(table), m_count(count), m_title(std::move(title)),
            m_invalid(std::move(invalid)) {}

        void operator()(Execution::Context& context) const override;

    private:
        Execution::TableOutputBase& m_table;
        size_t m_count;
        Resource::LocString m_title;
        Resource::LocString m_invalid;
    };

    // Handles all opened source(s) agreements if needed.
    // Required Args: The source to be checked for agreements
    // Inputs: None
    // Outputs: None
    struct HandleSourceAgreements : public WorkflowTask
    {
        HandleSourceAgreements(Repository::Source source) : WorkflowTask("HandleSourceAgreements"), m_source(std::move(source)) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::Source m_source;
    };

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
    // Inputs: PackageSubContexts
    // Outputs: None
    struct ShowPromptsForMultiplePackages : public WorkflowTask
    {
        ShowPromptsForMultiplePackages(bool ensureAgreementsAcceptance, bool installerDownloadOnly) :
            WorkflowTask("ShowPromptsForMultiplePackages"), m_ensureAgreementsAcceptance(ensureAgreementsAcceptance),
            m_installerDownloadOnly(installerDownloadOnly) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_ensureAgreementsAcceptance;
        bool m_installerDownloadOnly;
    };

    // If the context is not interactive, terminate it with the given HRESULT.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    struct RequireInteractivity : public WorkflowTask
    {
        RequireInteractivity(HRESULT nonInteractiveError) :
            WorkflowTask("RequireInteractivity"), m_nonInteractiveError(nonInteractiveError) {}

        void operator()(Execution::Context& context) const override;

    private:
        HRESULT m_nonInteractiveError;
    };
}